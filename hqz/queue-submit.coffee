#!/usr/bin/env coffee
#
#   This file is part of HQZ, the batch renderer for Zen Photon Garden.
#
#   Copyright (c) 2013 Micah Elizabeth Scott <micah@scanlime.org>
#
#   Permission is hereby granted, free of charge, to any person
#   obtaining a copy of this software and associated documentation
#   files (the "Software"), to deal in the Software without
#   restriction, including without limitation the rights to use,
#   copy, modify, merge, publish, distribute, sublicense, and/or sell
#   copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following
#   conditions:
#
#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#   OTHER DEALINGS IN THE SOFTWARE.
#

AWS = require 'aws-sdk'
async = require 'async'
util = require 'util'
fs = require 'fs'
crypto = require 'crypto'

sqs = new AWS.SQS({ apiVersion: '2012-11-05' }).client
s3 = new AWS.S3({ apiVersion: '2006-03-01' }).client

kRenderQueue = "zenphoton-hqz-render-queue"
kResultQueue = "zenphoton-hqz-results"
kBucketName = "hqz"
kKeyPrefix = "tmp-"

if process.argv.length < 3
    console.log "usage: queue-submit scene.json ..."
    process.exit 1

scenes = ( fs.readFileSync process.argv[i] for i in [2 .. process.argv.length - 1] )

async.waterfall [

    # Parallel initialization tasks
    (cb) ->
        async.parallel

            key: (cb) ->
                crypto.randomBytes 8, (error, data) ->
                    return cb error if error
                    cb error, kKeyPrefix + data.toString 'hex'

            renderQueue: (cb) ->
                sqs.createQueue
                    QueueName: kRenderQueue
                    cb

            resultQueue: (cb) ->
                sqs.createQueue
                    QueueName: kResultQueue
                    cb
            cb

    # Upload each scene in parallel
    (par, cb) ->
        async.map(
            [ "#{par.key}-#{i}", scenes[i] ] for i in [0 .. scenes.length-1]
            ([key, scene], cb) ->

                # Each upload step runs in series
                async.waterfall [

                    (cb) ->
                        s3.putObject
                            Bucket: kBucketName
                            ContentType: 'application/json'
                            Key: key + '.json'
                            Body: scene
                            cb

                    (data, cb) ->
                        obj =
                            SceneBucket: kBucketName
                            SceneKey: key + '.json'
                            OutputBucket: kBucketName
                            OutputKey: key + '.png'
                            OutputQueueUrl: par.resultQueue.QueueUrl

                        console.log key

                        sqs.sendMessage
                            QueueUrl: par.renderQueue.QueueUrl
                            MessageBody: JSON.stringify obj
                            cb
                ], cb
            cb
        )

], (error) -> 
    return console.log util.inspect error if error
    console.log "Job submitted"
