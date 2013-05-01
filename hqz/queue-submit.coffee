#!/usr/bin/env coffee
#
#   Job Submitter. Uploads the JSON scene description for a job
#   and enqueues a work item for each frame.
#
#   AWS configuration comes from the environment:
#
#      AWS_ACCESS_KEY_ID
#      AWS_SECRET_ACCESS_KEY
#      AWS_REGION
#      HQZ_BUCKET
#
#   Required Node modules:
#
#      npm install aws-sdk coffee-script async
#
######################################################################
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
path = require 'path'

sqs = new AWS.SQS({ apiVersion: '2012-11-05' }).client
s3 = new AWS.S3({ apiVersion: '2006-03-01' }).client

kRenderQueue = "zenphoton-hqz-render-queue"
kResultQueue = "zenphoton-hqz-results"
kBucketName = process.env.HQZ_BUCKET

if process.argv.length != 3
    console.log "usage: queue-submit JOBNAME.json"
    process.exit 1

jobName = path.basename process.argv[2], '.json'
rawInput = fs.readFileSync process.argv[2]
input = JSON.parse rawInput
inputHash = crypto.createHash('sha1').update(rawInput).digest('hex').slice(0, 8)
key = jobName + '/' + inputHash
jsonKey = key + '.json.gz'

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
        # Upload scene only if it isn't already on S3

        s3.headObject
            Bucket: kBucketName
            Key: jsonKey
            (error, data) ->

                if not error
                    console.log "Scene data already uploaded as #{jsonKey}"
                    cb error, obj

                else if error.code == 'NotFound'
                    console.log "Uploading scene data, #{obj.sceneData.length} bytes, as #{jsonKey}" 
                    s3.putObject
                        Bucket: kBucketName
                        ContentType: 'application/json'
                        Key: jsonKey
                        Body: obj.sceneData
                        (error, data) ->
                            return cb error if error
                            cb error, obj

                else
                    cb error

        # Prepare work items

        obj.work = for i in [0 .. frames.length - 1]
            Id: 'item-' + i
            MessageBody: JSON.stringify
                SceneBucket: kBucketName
                SceneKey: jsonKey
                SceneIndex: i
                OutputBucket: kBucketName
                OutputKey: key + '-' + pad(i, 4) + '.png'
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
