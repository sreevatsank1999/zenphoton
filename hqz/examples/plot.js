/*
 * plot.js -- A generic function plotter for HQZ. This module defines
 *            a function which, given a function describing a 2D curve,
 *            returns a list of HQZ objects describing that curve.
 * 
 * Optional arguments:
 *
 *   material:     Defaults to zero.
 *   resolution:   Distance between vertices, in scene units
 *   step:         Step size for numerical differentiation
 *
 *   The provided 'func' takes one argument, a float between 0 and 1.
 * It returns a 2-element [x, y] array.
 *
 * Micah Elizabeth Scott <micah@scanlime.org>
 *
 * Creative Commons BY-SA license:
 * http://creativecommons.org/licenses/by-sa/3.0/
 */

module.exports = function (options, func)
{
    var results = [];
    var material = options.material || 0;
    var resolution = options.resolution || 4.0;
    var step = options.step || 0.00001;
    var t = 0;
    var xp, yp, nxp, nyp;

    do {

        // Sample the function at two points
        var xy0 = func(t);
        var xy1 = func(t + step);

        // Numerical derivative
        var dx = (xy1[0]- xy0[0]) / step;
        var dy = (xy1[1] - xy0[1]) / step;

        // Length of derivative vector
        var dl = Math.sqrt(dx*dx + dy*dy);

        // Perpendicular normal vector at this point
        var nx = -dy / dl;
        var ny = dx / dl;

        // If we have at least two points, add a segment
        if (xp != undefined) {
            results.push([ material,
                xp, yp, xy0[0]-xp, xy0[1]-yp,
                nxp, nyp, nx-nxp, ny-nyp ]);
        }

        // Save "previous" values for the next point
        xp = xy0[0];
        yp = xy0[1];
        nxp = nx;
        nyp = ny;

        // Estimate how far to advance 't' by using the derivative to find a step
        // that should approximate the requested resolution in scene units.
        var adv = resolution / dl;

        // Limits on how quickly or slowly we can advance
        t += Math.min(0.1, Math.max(0.000001, adv));

    } while (t <= 1.0);

    return results;
}
