#!/usr/bin/env coffee
#
#   Watch the results of our rendering queue, and download the finished images.
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

log = (msg) -> console.log "[#{ (new Date).toJSON() }] #{msg}"
sqs = new AWS.SQS({ apiVersion: '2012-11-05' }).client
s3 = new AWS.S3({ apiVersion: '2006-03-01' }).client


class Watcher
    run: (queueName, cb) ->
        sqs.createQueue
            QueueName: queueName
            (error, data) =>
                if error
                    cb error
                if data
                    @queue = data.QueueUrl
                    @pollQueue()

    pollQueue: ->
        sqs.receiveMessage
            QueueUrl: @queue
            MaxNumberOfMessages: 10
            VisibilityTimeout: 120
            WaitTimeSeconds: 10

            (error, data) =>
                log "foobar"
                return log "Error reading queue: " + util.inspect error if error
                if data and data.Messages
                    for m in data.Messages
                        do (m) =>
                            cb = (error) => @messageComplete(error, m)
                            try
                                @handleMessage JSON.parse(m.Body), cb
                            catch error
                                cb error
                @pollQueue()

    messageComplete: (error, m) ->
        return log "Error processing message: " + util.inspect error if error
        sqs.deleteMessage
            QueueUrl: @queue
            ReceiptHandle: m.ReceiptHandle
            (error, data) =>
                @pollQueue()

    handleMessage: (msg, cb) ->
        console.log "#{ msg.State }# '#{ msg.SceneKey }'"
        if msg.State == 'finished'
            console.log "Downloading #{ msg.OutputKey }"

            o = s3.getObject
                Bucket: msg.OutputBucket
                Key: msg.OutputKey

            s = o.createReadStream()
            s.pipe fs.createWriteStream msg.OutputKey
            s.on 'end', () =>
                log "Finished downloading #{ msg.OutputKey }"
                cb()
        else
            cb()


qw = new Watcher
qw.run "zenphoton-hqz-results", (error) ->
    console.log util.inspect error
    process.exit 1
