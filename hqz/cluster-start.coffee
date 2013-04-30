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

# Tweakables

kRoleName       = 'hqz-node'
kSpotPrice      = "0.25"            # Maximum price per instance-hour
kInstanceCount  = 8
kImageId        = "ami-2efa9d47"    # Ubuntu 12.04 LTS, x64, us-east-1
kInstanceType   = "c1.xlarge"       # High-CPU instance

kInstanceTags   = [ {
    Key: "com.zenphoton.hqz"
    Value: "node"
} ]

######################################################################

AWS = require 'aws-sdk'
async = require 'async'
util = require 'util'

iam = new AWS.IAM({ apiVersion: '2010-05-08' }).client
ec2 = new AWS.EC2({ apiVersion: '2013-02-01' }).client
log = (msg) -> console.log "[#{ (new Date).toJSON() }] #{msg}"

script = """
    #!/bin/sh

    apt-get update
    apt-get install -y nodejs npm make gcc g++ git
    npm install -g coffee-script aws-sdk async

    ln -s /usr/bin/nodejs /usr/bin/node
    export NODE_PATH=/usr/local/lib/node_modules
    export AWS_REGION=#{ AWS.config.region }

    git clone https://github.com/scanlime/zenphoton.git
    cd zenphoton/hqz
    make

    while true; do
        ./queue-runner.coffee
    done
    """

assumeRolePolicy =
    Version: "2012-10-17"
    Statement: [
        Effect: "Allow"
        Principal:
            Service: [ "ec2.amazonaws.com" ]
        Action: [ "sts:AssumeRole" ]
    ]

policyDocument =
    Statement: [
        {
            Action: [ "s3:GetObject", "s3:PutObject" ]
            Effect: "Allow"
            Resource: [ "arn:aws:s3:::hqz/*" ]
        }, {
            Action: [
                "sqs:ChangeMessageVisibility"
                "sqs:CreateQueue"
                "sqs:DeleteMessage"
                "sqs:ReceiveMessage"
                "sqs:SendMessage"
            ]
            Effect: "Allow"
            Resource: [ "*" ]
        }
    ]

async.waterfall [

    # Create a security role for our new instances, if it doesn't already exist
    (cb) ->
        log "Setting up security role"
        iam.createRole
            RoleName: kRoleName
            AssumeRolePolicyDocument: JSON.stringify assumeRolePolicy
            (error, data) ->
                return cb null, {} if error and error.code == 'EntityAlreadyExists'
                return cb error if error

                iam.putRolePolicy
                    RoleName: kRoleName
                    PolicyName: "#{kRoleName}-policy"
                    PolicyDocument: JSON.stringify policyDocument
                    cb

    (data, cb) ->
        iam.createInstanceProfile
            InstanceProfileName: "#{kRoleName}-instance"
            (error, data) ->
                return cb null, {} if error and error.code == 'EntityAlreadyExists'
                return cb error if error

                iam.addRoleToInstanceProfile
                    InstanceProfileName: "#{kRoleName}-instance"
                    RoleName: kRoleName
                    cb

    # Request spot instances
    (data, cb) -> 
        log "Requesting spot instances"
        ec2.requestSpotInstances
            SpotPrice: kSpotPrice
            InstanceCount: kInstanceCount
            Type: "one-time"
            LaunchSpecification:
                ImageId: kImageId
                InstanceType: kInstanceType
                IamInstanceProfile:
                    Name: "#{kRoleName}-instance"
                UserData:
                    Buffer(script).toString('base64')
            cb

    # Tag each spot request
    (data, cb) ->
        requests = ( spot.SpotInstanceRequestId for spot in data.SpotInstanceRequests )
        log util.inspect requests
        ec2.createTags
            Resources: requests
            Tags: kInstanceTags
            cb

], (error, data) ->
    return log util.inspect error if error
    log "Cluster starting up!"
