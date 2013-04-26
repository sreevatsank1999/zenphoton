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
                checkMaterialID(object[0u]);
            }
        }
    }

    // Check all materials
    if (checkTuple(mMaterials, "materials", 0)) {
        for (unsigned i = 0; i < mMaterials.Size(); ++i)
            checkMaterialValue(i);
    }
}

void ZRender::render(std::vector<unsigned char> &pixels)
{
    unsigned numRays = sampleValue(mScene["rays"]);
    double exposure = sampleValue(mScene["exposure"]);

    for (unsigned i = numRays; i; --i)
        traceRay();

    double scale = exp(1.0 + 10.0 * exposure) / numRays;
    image.render(pixels, scale, 1.0);
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

bool ZRender::checkMaterialID(const Value &v)
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

bool ZRender::checkMaterialValue(int index)
{
    const Value &v = mMaterials[index];

    if (!v.IsArray()) {
        mError << "Material #" << index << " is not an array\n";
        return false;
    }

    bool result = true;
    for (unsigned i = 0, e = v.Size(); i != e; ++i) {
        const Value& outcome = v[i];

        if (!outcome.IsArray() || outcome.Size() < 1 || !outcome[0u].IsNumber()) {
            mError << "Material #" << index << " outcome #" << i << "is not an array starting with a number\n";
            result = false;
        }
    }

    return result;
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
    d.object = 0;

    double width = image.width();
    double height = image.height();

    // Sample the viewport once per ray. (e.g. for camera motion blur)
    ViewportSample v;
    initViewport(v);

    // Initialize the ray by sampling a light
    initRay(d.ray, chooseLight());

    // Look for a large but bounded number of bounces
    for (unsigned bounces = 1000; bounces; --bounces) {

        // Intersect with an object or the edge of the viewport
        bool hit = rayIntersect(d, v);

        // Draw a line from d.ray.origin to d.point
        image.line(128, 128, 128,
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

    r.origin.x = cartesianX + cos(polarAngle) * polarDistance;
    r.origin.y = cartesianY + sin(polarAngle) * polarDistance;

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

bool ZRender::rayIntersect(IntersectionData &d, const ViewportSample &v)
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
        const Value &object = mObjects[i];

        if (d.object != &object && rayIntersectObject(temp, object) && temp.distance < closest.distance) {
            closest = temp;
            closest.object = &object;
        }
    }

    if (closest.distance == DBL_MAX) {
        // Nothing. Intersect with the viewport bounds.
        rayIntersectBounds(d, v);
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
     *
     * Does not write to d.object; it is assumed that the caller does this.
     */

    if (object.Size() == 5) {
        // Line segment

        Vec2 origin = { sampleValue(object[1]), sampleValue(object[2]) };
        Vec2 delta = { sampleValue(object[3]), sampleValue(object[4]) };

        if (d.ray.intersectSegment(origin, delta, d.distance)) {
            d.point = d.ray.pointAtDistance(d.distance);
            d.normal.x = -delta.y;
            d.normal.y = delta.x;
            return true;
        }
    }

    return false;
}

void ZRender::rayIntersectBounds(IntersectionData &d, const ViewportSample &v)
{
    /*
     * This ray is never going to hit an object. Update d.point with the furthest away
     * viewport intersection, if any. If this ray never intersects the viewport, d.point
     * will equal d.ray.origin.
     */

    // Viewport bounds
    Vec2 topLeft = v.origin;
    Vec2 topRight = { v.origin.x + v.size.x, v.origin.y };
    Vec2 bottomLeft = { v.origin.x, v.origin.y + v.size.y };
    Vec2 horizontal = { v.size.x, 0 };
    Vec2 vertical = { 0, v.size.y };

    double dist, furthest = 0;
    if (d.ray.intersectSegment(topLeft, horizontal, dist)) furthest = std::max(furthest, dist);
    if (d.ray.intersectSegment(bottomLeft, horizontal, dist)) furthest = std::max(furthest, dist);
    if (d.ray.intersectSegment(topLeft, vertical, dist)) furthest = std::max(furthest, dist);
    if (d.ray.intersectSegment(topRight, vertical, dist)) furthest = std::max(furthest, dist);

    d.point = d.ray.pointAtDistance(furthest);
}

bool ZRender::rayMaterial(IntersectionData &d)
{
    /*
     * Sample the material indicated by the intersection 'd' and update the ray.
     * Returns 'true' if the ray continues propagating, or 'false' if it's abosrbed.
     */

    // Lookup in our material database
    const Value &object = *d.object;
    unsigned id = object[0u].GetUint();
    const Value &material = mMaterials[id];

    double r = mRandom.uniform();
    double sum = 0;

    // Loop over all material outcomes, pick one according to our random variable.
    for (unsigned i = 0, e = material.Size(); i != e; ++i) {
        const Value& outcome = material[i];
        sum += outcome[0u].GetDouble();
        if (r <= sum)
            return rayMaterialOutcome(d, outcome);
    }

    // Absorbed
    return false;
}

bool ZRender::rayMaterialOutcome(IntersectionData &d, const Value &outcome)
{
    // Check for 2-tuple outcomes
    if (outcome.IsArray() && outcome.Size() == 2) {

        // Look for 2-tuples with a character parameter
        const Value &param = outcome[1];
        if (param.IsString() && param.GetStringLength() == 1) {
            switch (param.GetString()[0]) {

                // Perfectly diffuse, emit the ray with a random angle.
                case 'd':
                    d.ray.origin = d.point;
                    d.ray.setAngle(mRandom.uniform(0, M_PI * 2.0));
                    return true;

                // Perfectly transparent, emit the ray with no change in angle.
                case 't':
                    d.ray.origin = d.point;
                    return true;

                // Reflected back according to the object's surface normal
                case 'r':
                    d.ray.origin = d.point;
                    d.ray.reflect(d.normal);
                    return true;

            }
        }
    }

    // Unknown outcome
    return false;
}
