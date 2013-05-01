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
outputFile = "queue-watcher.log"

sqs = new AWS.SQS({ apiVersion: '2012-11-05' }).client
s3 = new AWS.S3({ apiVersion: '2006-03-01' }).client

fileLog = (msg) ->
    out = fs.createWriteStream outputFile, {flags: 'a'}
    out.end msg + '\n'


class Watcher
    constructor: ->
        @jobs = {}

    replaySync: (filename, cb) ->
        try
            lines = fs.readFileSync(filename).toString().split('\n')
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

    logAndHandleMessage: (msg, cb) ->
        fileLog JSON.stringify msg
        @handleMessage msg, cb

    handleMessage: (msg, cb) ->
        @jobs[msg.SceneKey] = [] if not @jobs[msg.SceneKey]
        job = @jobs[msg.SceneKey]
        index = 0
        index = msg.SceneIndex if msg.SceneIndex > 0

        # Do we have a new URL to show?
        if msg.State == 'finished'
            log "http://#{ msg.OutputBucket }.s3.amazonaws.com/#{ msg.OutputKey }"

        # Update the state of this render job
        job[index] = msg.State

        # Summarize the job state
        summary = for i in [0 .. job.length - 1]
            switch job[i]
                when 'finished' then '#'
                when 'started' then '.'
                when 'failed' then '!'
                else ' '

        log "[#{ summary.join '' }] -- #{msg.SceneKey} [#{index}] #{msg.State}"
        cb()


cb = (error) -> log util.inspect error if error
qw = new Watcher
qw.replaySync outputFile, cb
qw.run "zenphoton-hqz-results"
