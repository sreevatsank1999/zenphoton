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
#      npm install aws-sdk coffee-script async clarinet
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
clarinet = require 'clarinet'

sqs = new AWS.SQS({ apiVersion: '2012-11-05' }).client
s3 = new AWS.S3({ apiVersion: '2006-03-01' }).client

kRenderQueue = "zenphoton-hqz-render-queue"
kResultQueue = "zenphoton-hqz-results"
kBucketName = process.env.HQZ_BUCKET


pad = (str, length) ->
    str = '' + str
    str = '0' + str while str.length < length
    return str

bufferConcat = (list) ->
    # Just like Buffer.concat(), but compatible with older versions of node.js
    size = 0
    for buf in list
        size += buf.length
    result = new Buffer size
    offset = 0
    for buf in list
        buf.copy(result, offset)
        offset += buf.length
    return result


if process.argv.length != 3
    console.log "usage: queue-submit JOBNAME.json"
    process.exit 1

filename = process.argv[2]
console.log "Reading #{filename}..."

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

            # Truncated sha1 hash of input file
            hash: (cb) ->
                hash = crypto.createHash 'sha1'
                s = fs.createReadStream filename
                s.on 'data', (chunk) -> hash.update chunk
                s.on 'end', () ->
                    h = hash.digest 'hex'
                    console.log "    sha1 #{h}"
                    cb null, h.slice(0, 8)

            # Gzipped in-memory input buffer
            gzScene: (cb) ->
                gz = zlib.createGzip()
                chunks = []
                s = fs.createReadStream filename
                s.pipe gz
                gz.on 'data', (chunk) -> chunks.push chunk
                gz.on 'end', () ->
                    data = bufferConcat chunks
                    console.log "    compressed to #{data.length} bytes"
                    cb null, data

            # Number of frames, or null if this isn't an animation
            frameCount: (cb) ->
                level = 0
                frames = 0
                isSingleFrame = true
                stream = clarinet.createStream()
                fs.createReadStream(filename).pipe(stream)

                stream.on 'openobject', (key) ->
                    frames++ if level == 1 and !isSingleFrame
                    level++

                stream.on 'closeobject', () ->
                    level--

                stream.on 'openarray', () ->
                    isSingleFrame = false if level == 0
                    level++

                stream.on 'closearray', () ->
                    level--

                stream.on 'error', (e) ->
                    cb error

                stream.on 'end', (e) ->
                    if isSingleFrame
                        cb null, null
                        console.log "    rendering a single frame"
                    else
                        cb null, frames
                        console.log "    animation with #{ frames } frames"
            cb

    (obj, cb) ->
        # Upload scene only if it isn't already on S3

        # Create a unique identifier for this job
        jobName = path.basename filename, '.json'
        obj.key = jobName + '/' + obj.hash
        obj.jsonKey = obj.key + '.json.gz'

        s3.headObject
            Bucket: kBucketName
            Key: obj.jsonKey
            (error, data) ->

                if not error
                    console.log "Scene data already uploaded as #{obj.jsonKey}"
                    cb error, obj

                else if error.code == 'NotFound'
                    console.log "Uploading scene as #{obj.jsonKey}"
                    s3.putObject
                        Bucket: kBucketName
                        ContentType: 'application/json'
                        Key: obj.jsonKey
                        Body: obj.gzScene
                        (error, data) ->
                            return cb error if error
                            cb error, obj

                else
                    cb error

        # Prepare work items

        if obj.frameCount == null
            obj.work = [
                Id: 'single'
                MessageBody: JSON.stringify
                    SceneBucket: kBucketName
                    SceneKey: obj.jsonKey
                    OutputBucket: kBucketName
                    OutputKey: obj.key + '.png'
                    OutputQueueUrl: obj.resultQueue.QueueUrl
            ]
        else
            obj.work = for i in [0 .. obj.frameCount - 1]
                Id: 'item-' + i
                MessageBody: JSON.stringify
                    SceneBucket: kBucketName
                    SceneKey: obj.jsonKey
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
