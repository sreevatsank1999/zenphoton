#!/usr/bin/env coffee
#
#   Cluster startup script. Experimental!
#
#   AWS configuration comes from the environment:
#
#      AWS_ACCESS_KEY_ID
#      AWS_SECRET_ACCESS_KEY
#      AWS_REGION
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

ec2 = new AWS.EC2().client
log = (msg) -> console.log "[#{ (new Date).toJSON() }] #{msg}"

script = """
    #!/bin/sh

    sudo apt-get update
    sudo apt-get install -y nodejs npm make gcc g++ git
    sudo ln -s /usr/bin/nodejs /usr/bin/node
    sudo npm install -g coffee-script

    git clone https://github.com/scanlime/zenphoton.git

    cd zenphoton/hqz
    make

    export AWS_ACCESS_KEY_ID=#{ AWS.config.credentials.accessKeyId }
    export AWS_SECRET_ACCESS_KEY=#{ AWS.config.credentials.secretAccessKey }
    export AWS_REGION=#{ AWS.config.region }

    while true; do
        ./queue-runner.coffee
    done
    """

console.log script

ec2.requestSpotInstances
    SpotPrice: "0.25"
    InstanceCount: 1
    Type: "one-time"
    LaunchSpecification:
        ImageId: "ami-2c002c69"     # Ubuntu 12.04 LTS, x64, us-west-1
        InstanceType: "c1.xlarge"   # High-CPU instance
        UserData:
            Buffer(script).toString('base64')

    (error, data) ->
        return log util.inspect error if error
        log "Success."
        log util.inspect data
