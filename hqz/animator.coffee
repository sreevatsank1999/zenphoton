#!/usr/bin/env coffee
#
# Simple animation test, using node.js to spawn rendering processes.
#

scene = 
    resolution: [1024, 576]
    rays: 500000
    viewport: [0, 0, 1024, 576]
    exposure: 0.7
    gamma: 1.8
    lights: [
        [1.0, 697, 1, 0, 0, [0, 360],
            [5000, "K"]
        ]
    ]
    objects: [
        [0, 2, 527, 1024, -26],
        [0, 249, 524, -35, -316],
        [0, 235, 386, 55, -112],
        [0, 234, 364, -78, -75],
        [0, 278, 305, -20, -72],
        [0, 269, 259, 17, -66],
        [0, 283, 213, -49, -62],
        [0, 220, 246, -63, -60],
        [0, 179, 209, -10, -73],
        [0, 223, 293, -77, -38],
        [0, 241, 432, 13, -31],
        [0, 257, 346, -3, -42],
        [0, 266, 323, 21, 0],
        [0, 239, 406, -41, -45],
        [0, 244, 483, -24, -49],
        [0, 242, 456, 1, -14],
        [0, 322, 335, 27, 34],
        [0, 335, 333, 35, 40],
        [0, 353, 323, 35, 39],
        [0, 372, 317, 29, 24],
        [0, 384, 297, 29, 37],
        [0, 415, 314, 31, 24],
        [0, 449, 339, 19, 0],
        [0, 471, 340, 13, 14],
        [0, 484, 344, 22, 6],
        [0, 513, 341, 3, -9],
        [0, 532, 349, 16, 26],
        [0, 549, 363, 5, 5],
        [0, 554, 365, 7, 30],
        [0, 586, 364, 10, 13],
        [0, 592, 345, 35, 46],
        [0, 631, 367, 32, 16],
        [0, 673, 381, 23, 12],
        [0, 696, 392, 5, 22],
        [0, 545, 208, 62, 0],
        [0, 730, 202, 37, 0],
        [0, 865, 303, 50, -9],
        [0, 871, 204, 33, -4],
        [0, 893, 56, 24, -1],
        [0, 410, 75, 10, 2],
        [0, 441, 103, 12, 6]
    ]
    materials: [
        [[1, "d"]]
    ]


spawn = require('child_process').spawn
numCPUs = require('os').cpus().length

pad = (str, length) ->
    str = '0' + str while str.length < length
    return str

frameQueue = for x in [0 .. 1024]
    scene.lights[0][1] = x
    [ "frame-#{ pad(''+x, 4) }.png", JSON.stringify(scene) ]

startWorker = () ->
    return if not frameQueue
    [ filename, scene ] = frameQueue.shift()
    console.log "Starting #{filename}"
    child = spawn './hqz', ['-', filename]
    child.on 'exit', startWorker
    child.stdin.write scene
    child.stdin.end()

for n in [1 .. numCPUs]
    startWorker()
