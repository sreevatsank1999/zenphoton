/*
 * This file is part of HQZ, the batch renderer for Zen Photon Garden.
 *
 * Copyright (c) 2013 Micah Elizabeth Scott <micah@scanlime.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <float.h>
#include "zrender.h"


ZRender::ZRender(const Value &scene)
    : mScene(scene),
    mViewport(scene["viewport"]),
    mLights(scene["lights"]),
    mObjects(scene["objects"]),
    mMaterials(scene["materials"]),
    mLightPower(0.0)
{
    // Seed the PRNG. (Zero by default)
    mRandom.seed(sampleValue(mScene["seed"]));

    // Integer resolution values
    const Value& resolution = mScene["resolution"];
    if (checkTuple(resolution, "resolution", 2)) {
        image.resize(sampleValue(resolution[0u]), sampleValue(resolution[1]));
    }

    // Other cached tuples
    checkTuple(mViewport, "viewport", 4);
    checkTuple(mMaterials, "materials", 0);

    // Add up the total light power in the scene, and check all lights.
    if (checkTuple(mLights, "viewport", 1)) {
        for (unsigned i = 0; i < mLights.Size(); ++i) {
            const Value &light = mLights[i];
            if (checkTuple(light, "light", 7))
                mLightPower += sampleValue(light[0u]);
        }
    }
    if (mLightPower <= 0.0) {
        mError << "Total light power (" << mLightPower << ") must be positive.\n";
    }

    // Check all objects
    if (checkTuple(mObjects, "objects", 0)) {
        for (unsigned i = 0; i < mObjects.Size(); ++i) {
            const Value &object = mObjects[i];
            if (checkTuple(object, "object", 5)) {
                checkMaterial(object[0u]);
            }
        }
    }
}

void ZRender::render(std::vector<unsigned char> &pixels)
{
    for (unsigned i = 0, e = sampleValue(mScene["rays"]); i != e; ++i)
        traceRay();

    image.render(pixels, 0.0005, 2.0);
}

bool ZRender::checkTuple(const Value &v, const char *noun, unsigned expected)
{
    /*
     * A quick way to check for a tuple with an expected length, and set
     * an error if there's any problem. Returns true on success.
     */

    if (!v.IsArray() || v.Size() < expected) {
        mError << "'" << noun << "' expected an array with at least "
            << expected << " item" << (expected == 1 ? "" : "s") << "\n";
        return false;
    }

    return true;
}

bool ZRender::checkMaterial(const Value &v)
{
    // Check for a valid material ID.

    if (!v.IsUint()) {
        mError << "Material ID must be an unsigned integer\n";
        return false;
    }

    if (v.GetUint() >= mMaterials.Size()) {
        mError << "Material ID (" << v.GetUint() << ") out of range\n";
        return false;
    }

    return true;
}

double ZRender::sampleValue(const Value &v)
{
    /*
     * Stochastically sample a JSON value which may be a random variable.
     * 'v' may be any of the following JSON constructs:
     *
     *      1.0             A single number. Always returns this value.
     *      null            Synonymous with zero.
     *      [ 1.0, 5.0 ]    A list of exactly two numbers. Samples uniformly.
     *      others          Reserved for future definition. (Zero)
     */

    if (v.IsNumber())
        return v.GetDouble();

    if (v.IsArray() && v.Size() == 2 && v[0U].IsNumber() && v[1].IsNumber())
        return mRandom.uniform(v[0U].GetDouble(), v[1].GetDouble());

    return 0;
}

const ZRender::Value& ZRender::chooseLight()
{
    // Pick a random light, using the light power as a probability weight.
    // Fast path for scenes with only one light.

    unsigned i = 0;
    unsigned last = mLights.Size() - 1;

    if (i != last) {
        double r = mRandom.uniform(0, mLightPower);
        double sum = 0;

        // Check all lights except the last
        do {
            const Value& light = mLights[i++];
            sum += sampleValue(light[0u]);
            if (r <= sum)
                return light;
        } while (i != last);
    }

    // Default, last light.
    return mLights[last];
}

void ZRender::traceRay()
{
    IntersectionData d;
    ViewportSample v;
    double width = image.width();
    double height = image.height();

    // Sample the viewport once per ray. (e.g. for camera motion blur)
    initViewport(v);

    // Initialize the ray by sampling a light
    initRay(d.ray, chooseLight());

    // Look for a large but bounded number of bounces
    for (unsigned bounces = 1000; bounces; --bounces) {

        // Intersect with an object or the edge of the viewport
        bool hit = rayIntersect(d);

        // Draw a line from d.ray.origin to d.point
        image.line(127, 127, 127,
            v.xScale(d.ray.origin.x, width),
            v.yScale(d.ray.origin.y, height),
            v.xScale(d.point.x, width),
            v.yScale(d.point.y, height));

        if (!hit) {
            // Ray exited the scene after this.
            break;
        }

        if (!rayMaterial(d)) {
            // Ray was absorbed by material
            break;
        }
    }
}

void ZRender::initRay(Ray &r, const Value &light)
{
    double cartesianX = sampleValue(light[1]);
    double cartesianY = sampleValue(light[2]);
    double polarAngle = sampleValue(light[3]) * (M_PI / 180.0);
    double polarDistance = sampleValue(light[4]);
    double rayAngle = sampleValue(light[5]) * (M_PI / 180.0);
    double wavelength = sampleValue(light[6]);

    r.setOrigin( cartesianX + cos(polarAngle) * polarDistance,
                 cartesianY + sin(polarAngle) * polarDistance );
    r.setAngle(rayAngle);
    r.wavelength = wavelength;
}

void ZRender::initViewport(ViewportSample &v)
{
    // Sample the viewport. We do this once per ray.

    v.origin.x = sampleValue(mViewport[0u]);
    v.origin.y = sampleValue(mViewport[1]);
    v.size.x = sampleValue(mViewport[2]);
    v.size.y = sampleValue(mViewport[3]);
}

bool ZRender::rayMaterial(IntersectionData &d)
{
    // Sample the material indicated by the intersection 'd' and update the ray.
    // Returns 'true' if the ray continues propagating, or 'false' if it's abosrbed.

    return false;
}

bool ZRender::rayIntersect(IntersectionData &d)
{
    /*
     * Sample all objects in the scene that d.ray might hit. If we hit an object,
     * updates 'd' and returns true. If we don't, d.point will be clipped to the
     * edge of the image by rayIntersectBounds() and we return 'false'.
     *
     * Currently this scans all objects each time, since we don't have a scene partitioning
     * data structure yet which can deal with randomly sampled coordinates.
     */

    IntersectionData temp = d;
    IntersectionData closest;
    closest.distance = DBL_MAX;

    for (unsigned i = 0, e = mObjects.Size(); i != e; ++i) {
        if (rayIntersectObject(temp, mObjects[i])) {
            if (temp.distance < closest.distance)
                closest = temp;
        }
    }

    if (closest.distance == DBL_MAX) {
        // Nothing. Intersect with the viewport bounds.
        rayIntersectBounds(d);
        return false;
    }

    d = closest;
    return true;
}

bool ZRender::rayIntersectObject(IntersectionData &d, const Value &object)
{
    /*
     * Does this ray intersect a specific object? This samples the object once,
     * filling in the required IntersectionData on success. If this ray and object
     * don't intersect at all, returns false.
     */

     return false;
}

void ZRender::rayIntersectBounds(IntersectionData &d)
{
    /*
     * This ray is never going to hit an object. Update d.point with the point where
     * this ray leaves the viewport bounds. If the ray is outside the viewport and it will
     * never come back, this sets d.point = d.ray.origin.
     */

    d.point.x = 50;
    d.point.y = 50;
}
