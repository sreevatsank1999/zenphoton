#!/usr/bin/env coffee
#
# Animation script for HQZ.
# Micah Elizabeth Scott <micah@scanlime.org>
# Creative Commons BY-SA license:
# http://creativecommons.org/licenses/by-sa/3.0/
#

TAU = Math.PI * 2

drawBranches = (frame, x0, y0, dx, dy) ->
    len = Math.sqrt(dx * dx + dy * dy)
    return [] if len < 10

    angle = Math.atan2(dy, dx)
    angle += 0.01 * Math.sin(frame * TAU / 100.0)

    # Branching distance, angle delta, scale

    d1 = 0.7
    a1 = -29 * TAU / 360
    s1 = 0.7

    d2 = 0.7
    a2 = 32 * TAU / 360
    s2 = 0.7

    return [
        [0, x0, y0, dx, dy]
    ].concat(
        drawBranches frame, x0 + dx * d1, y0 + dy * d1, len * s1 * Math.cos(angle + a1), len * s1 * Math.sin(angle + a1)
        drawBranches frame, x0 + dx * d2, y0 + dy * d2, len * s2 * Math.cos(angle + a2), len * s2 * Math.sin(angle + a2)
    )

floor = [0, -100, 900, 2200, -50]

floorY = (floorX) ->
    floor[4] / floor[3] * (floorX - floor[1]) + floor[2]

drawTree = (frame, x, dx, dy) ->
    drawBranches frame, x, floorY(x), dx, dy

sunlight = (frame) ->
    angle = frame * 0.05
    [ 1, 2000, -20, 0, 0, [angle + 90, angle + 180], [5000, 'K'] ]

skylight = (frame) ->
    angle = 90
    spread = 20
    x0 = -100
    x1 = 1920 + 100
    [ 0.8, [x0, x1], -30, 0, 0, [angle - spread, angle + spread], [8000, 'K'] ]

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

frames = for frame in [0 .. 4]

    resolution: [960, 540]
    rays: 100000
    exposure: 0.65

    viewport: zoomViewport 1920, 1080, 324, 178, frame * 0.01 
    seed: frame * 10

    lights: [
        sunlight frame
        skylight frame
    ]

    materials: [
        [ [0.9, "d"], [0.1, "r"] ]                  # 0. Floor
        [ [0.4, "t"], [0.4, "d"], [0.1, "r"] ]      # 1. Branches
    ]

    objects: [
        floor
    ].concat(
        drawTree frame, 500, -30, -300
        drawTree frame, 900, 5, -100 
        drawTree frame, 1300, 12, -180 
    )

console.log JSON.stringify frames
