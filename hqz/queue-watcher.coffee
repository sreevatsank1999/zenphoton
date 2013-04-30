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
os = require 'os'

log = (msg) -> console.log "[#{ (new Date).toJSON() }] #{msg}"
outputFile = "queue-watcher.log"

sqs = new AWS.SQS({ apiVersion: '2012-11-05' }).client
s3 = new AWS.S3({ apiVersion: '2006-03-01' }).client

fileLog = (msg) ->
    out = fs.createWriteStream outputFile, {flags: 'a'}
    out.end msg + os.EOL


class Watcher
    constructor: ->
        @numStarted = 0
        @numFinished = 0

    replaySync: (filename, cb) ->
        try
            lines = fs.readFileSync(filename).toString().split(os.EOL)
        catch error
            # Okay if this file doesn't exist
            cb error if error.code != 'ENOENT'
            return

        for line in lines
            if line.length and line[0] == '{'
                try
                    @handleMessage JSON.parse(line), cb
                catch error
                    cb error

    run: (queueName, cb) ->
        sqs.createQueue
            QueueName: queueName
            (error, data) =>
                if error
                    cb error
                if data
                    @queue = data.QueueUrl
                    log 'Watching for results...'
                    @pollQueue()

    pollQueue: ->
        sqs.receiveMessage
            QueueUrl: @queue
            MaxNumberOfMessages: 10
            VisibilityTimeout: 120
            WaitTimeSeconds: 10

            (error, data) =>
                return log error if error
                if data and data.Messages
                    for m in data.Messages
                        do (m) =>
                            cb = (error) => @messageComplete(error, m)
                            try
                                @logAndHandleMessage JSON.parse(m.Body), cb
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

    messageURL: (msg) ->
        if msg.State == 'finished'
            return "http://#{ msg.OutputBucket }.s3.amazonaws.com/#{ msg.OutputKey }"

    logAndHandleMessage: (msg, cb) ->
        fileLog JSON.stringify msg
        url = @messageURL msg
        fileLog url if url
        @handleMessage msg, cb

    handleMessage: (msg, cb) ->
        @numStarted += 1 if msg.State == 'started'
        @numFinished += 1 if msg.State == 'finished'
        arg = @messageURL(msg) or msg.SceneKey
        log "[Seen: #{@numStarted}+ / #{@numFinished}-] -- #{ msg.State } -- #{ arg }"
        cb()


cb = (error) -> log util.inspect error if error
qw = new Watcher
qw.replaySync outputFile, cb
qw.run "zenphoton-hqz-results"
