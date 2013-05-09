#!/usr/bin/env coffee
#
# Animation script for HQZ.
# Micah Elizabeth Scott <micah@scanlime.org>
# Creative Commons BY-SA license:
# http://creativecommons.org/licenses/by-sa/3.0/
#

plot = require './plot'
arc4rand = require 'arc4rand'
TAU = Math.PI * 2

harmonicOscillator = (frame, opts) ->
    rnd = (new arc4rand(opts.seed)).random

    # Calculate series coefficients once per frame
    series = for i in [1 .. 16]
        freq: i
        amplitude: rnd(0, 1.0) / i
        phase: rnd(0, TAU) + rnd(-0.01, 0.01) * frame

    plot opts, (t) ->
        # Draw a truncated Fourier series, in polar coordinates
        t *= TAU
        r = 0
        for s in series
            r += s.amplitude * Math.sin( s.freq * (t + s.phase) )
        r = opts.radius + r * opts.radius * opts.modulationDepth
        [ opts.x0 + r * Math.cos(t), opts.y0 + r * Math.sin(t) ]

zoomViewport = (width, height, focusX, focusY, zoom) ->
    left = focusX
    right = width - focusX
    top = focusY
    bottom = height - focusY
    scale = 1.0 - zoom

    left = focusX - left * scale
    right = focusX + right * scale
    top = focusY - top * scale
    bottom = focusY + bottom * scale

    [ left, top, right - left, bottom - top ]

focusX = 1920 * 0.8
focusY = 1080/3

scene = (frame) ->
    resolution: [1920, 1080]
    timelimit: 60 * 60
    rays: 5000000
    seed: frame * 100000

    exposure: 0.7
    gamma: 1.8
    viewport: zoomViewport 1920, 1080, 1920 * 0.8, 1080/3, frame * 0.001

    lights: [
        [0.75, 1920/2, 1080/2, [0, -180], 1200, [0, 360], 480]
        [0.2, 1920/2, 1080/2, [0, -180], 1200, [0, 360], 0]
        [0.1, focusX, focusY, [0, 380], [0, 15], [0, 360], 590]
    ]

    objects: [].concat [],

        harmonicOscillator frame,
            material: 0
            seed: 's'
            x0: 1920 * 0.4
            y0: 1080 * 2/3
            radius: 500
            modulationDepth: 0.25

        harmonicOscillator frame,
            material: 1    
            seed: 't'
            x0: focusX
            y0: focusY
            radius: 120
            modulationDepth: 0.25

    materials: [
        # 0. Light catching. Lots of internal reflection.
        [ [0.1, "d"], [0.8, "r"] ]

        # 1. Light emitting. Diffuse the light, don't reflect too much or we'll amplify
        #    nonuniformities in our circle more than I'd like.
        [ [1.0, "d"] ]
    ]

console.log JSON.stringify scene i for i in [0 .. 600]
#console.log JSON.stringify scene 0
