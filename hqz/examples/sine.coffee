#!/usr/bin/env coffee
#
# Animation script for HQZ.
# Micah Elizabeth Scott <micah@scanlime.org>
# Creative Commons BY-SA license:
# http://creativecommons.org/licenses/by-sa/3.0/
#

RAYS = 10000000

TAU = Math.PI * 2
deg = (degrees) -> degrees * TAU / 360
plot = require './plot'

lerp = (frame, length, a, b) ->
    a + (b - a) * frame / length

sunlight = (frame) ->
    # Warm point source
    x = lerp frame, 600, 1920, -500
    [ 1.0, x, -20, 0, 0, [0, 180], [5000, 'K'] ]

sealight = (frame) ->
    # Big diffuse light, with a blueish color
    x = lerp frame, 600, 1920, -200
    [ 1.0, [x-500, x+500], 1100, 0, 0, [180, 360], [10000, 'K'] ]

sineFunc = (frame, seed, x0, y0, dx, dy) ->
    (t) ->
        e = lerp frame, 600, 300, 50
        u = lerp frame, 600, 20, 10
        scale = Math.pow(e, t)
        [ x0 + dx * t, y0 + dy * Math.sin(t*(u + scale)) / (1 + scale) ]

frames = for frame in [0 .. 599]

    resolution: [1920, 1080]
    rays: RAYS
    exposure: 0.65
    gamma: 2.2

    viewport: [0, 0, 1920, 1080]
    seed: frame * RAYS / 20

    lights: [
        sunlight frame
        sealight frame
    ]

    materials: [
        [ [0.1, "d"], [0.9, "r"] ]
    ]

    objects: [
        # No fixed objects
    ].concat(
        plot
            material: 0
            sineFunc frame, '1', 50, 500, 1820, 900
    )

console.log JSON.stringify frames
