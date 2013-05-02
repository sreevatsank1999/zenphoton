#!/usr/bin/env coffee
#
# Animation script for HQZ.
# Micah Elizabeth Scott <micah@scanlime.org>
# Creative Commons BY-SA license:
# http://creativecommons.org/licenses/by-sa/3.0/
#

RAYS = 1000000

TAU = Math.PI * 2
deg = (degrees) -> degrees * TAU / 360

lightPairX = (frame, radius) ->
    x = 1800 - frame * 3.0
    [ x - radius, x + radius ]

sunlight = (frame) ->
    # Just spread enough to soften the bright rays on the corners of our line segments :(
    [ 1.0, lightPairX(frame, 80.0), -20, 0, 0, [0, 180], [5000, 'K'] ]

sealight = (frame) ->
    # Big diffuse light, with a blueish color
    [ 1.0, lightPairX(frame, 500.0), 1100, 0, 0, [180, 360], [10000, 'K'] ]

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

drawSine = (frame, seed, x0, y0, dx, dy) ->
    [ x, y ] = [ x0, y0 ]
    for t in [0 .. 1.0] by 0.001
        [ xp, yp ] = [ x, y ]
        scale = Math.pow(300, t)
        w = t * (30 - frame * 15.0 / 600)
        [ x, y ] = [ x0 + dx * t, y0 + dy * Math.sin(w + scale) / (1 + scale) ]
        [ 0, xp, yp, x - xp, y - yp ]

frames = for frame in [0 .. 599]

    resolution: [1920/2, 1080/2]
    rays: RAYS
    exposure: 0.65

    viewport: zoomViewport 1920, 1080, 324, 178, frame * 0.0003
    seed: frame * RAYS / 50

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
        drawSine frame, '1', -100, 500, 2500, 500
    )

console.log JSON.stringify frames[0]
