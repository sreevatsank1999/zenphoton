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
zlib = require 'zlib'

sqs = new AWS.SQS({ apiVersion: '2012-11-05' }).client
s3 = new AWS.S3({ apiVersion: '2006-03-01' }).client

kRenderQueue = "zenphoton-hqz-render-queue"
kResultQueue = "zenphoton-hqz-results"
kBucketName = "hqz"

if process.argv.length != 4
    console.log "usage: queue-submit job-name (scene.json | framelist.json)"
    process.exit 1

jobName = process.argv[2]
input = JSON.parse fs.readFileSync process.argv[3]

if input.length
    console.log "Found an animation with #{ input.length } frames"
    frames = input
else
    console.log "Rendering a single frame"
    frames = [ input ]

pad = (str, length) ->
    str = '' + str
    str = '0' + str while str.length < length
    return str

async.waterfall [

    # Parallel initialization tasks
    (cb) ->
        async.parallel

            key: (cb) ->
                crypto.randomBytes 4, (error, data) ->
                    return cb error if error
                    cb error, jobName + '/' + data.toString 'hex'

            renderQueue: (cb) ->
                sqs.createQueue
                    QueueName: kRenderQueue
                    cb

            resultQueue: (cb) ->
                sqs.createQueue
                    QueueName: kResultQueue
                    cb

            sceneData: (cb) ->
                zlib.gzip JSON.stringify(frames), cb

            cb

    (obj, cb) ->
        # Upload scene

        name = obj.key + '.json.gz'
        console.log "Uploading scene data, #{obj.sceneData.length} bytes, as #{name}" 
        s3.putObject
            Bucket: kBucketName
            ContentType: 'application/json'
            Key: name
            Body: obj.sceneData
            (error, data) ->
                return cb error if error
                cb error, obj

        # Prepare work items

        obj.work = for i in [0 .. frames.length - 1]
            Id: 'item-' + i
            MessageBody: JSON.stringify
                SceneBucket: kBucketName
                SceneKey: obj.key + '.json.gz'
                SceneIndex: i
                OutputBucket: kBucketName
                OutputKey: obj.key + '-' + pad(i, 4) + '.png'
                OutputQueueUrl: obj.resultQueue.QueueUrl

    # Enqueue work items
    (obj, cb) ->
        async.whilst(
            () -> obj.work.length > 0
            (cb) ->
                console.log "Enqueueing work items, #{obj.work.length} remaining"
                batch = Math.min(10, obj.work.length)
                thisBatch = obj.work.slice(0, batch)
                obj.work = obj.work.slice(batch)
                sqs.sendMessageBatch
                    QueueUrl: obj.renderQueue.QueueUrl
                    Entries: thisBatch
                    cb
            cb
        )


], (error) -> 
    return console.log util.inspect error if error
    console.log "Job submitted"
