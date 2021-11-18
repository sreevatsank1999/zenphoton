
#include <float.h>
#include <time.h>
#include "ztrace.h"
#include "zcheck.h"
#include "zmaterial.h"

ZTrace::ZTrace(const Value &scene)
    : mLights(scene["lights"]),
    mObjects(scene["objects"]),
    mMaterials(scene["materials"]),
    mLightPower(0.0)
{
    // Todo
}

std::vector<Path> ZTrace::traceRays()
{
    /*
     * Keep tracing rays until a stopping condition is hit.
     * Returns the total number of rays traced.
     */
    
    double startTime = (double)time(0);

    std::vector<Path> paths;    paths.reserve(mRayLimit);

    uint64_t batchsize = 1000;

    for(uint64_t rayCount=0; mRayLimit<mRayLimit; rayCount += batchsize) {
        // Minimum frequency for checking stopping conditions

        if (mTimeLimit) {
            // Check time limit
            double now = (double)time(0);
            if (now > startTime + mTimeLimit)
                break;
        }

        std::vector<Path> path_batch = traceRayBatch(mSeed + rayCount, batchsize);
        paths.insert(paths.end(), std::make_move_iterator(path_batch.begin()),
                                  std::make_move_iterator(path_batch.end()));
    }

    return paths;
}

std::vector<Path> ZTrace::traceRayBatch(uint32_t seed, uint32_t count)
{
    /*
     * Trace a batchsize of rays, starting with ray number "start", and
     * tracing "count" rays.
     *
     * Note that each ray is seeded separately, so that rays are independent events
     * with respect to the PRNG sequence. This helps keep our noise pattern stationary,
     * which is a nice effect to have during animation.
     */

    while (count--) {
        Sampler s(seed++);
        traceRay(s);
    }
}

Path ZTrace::traceRay(Sampler &s)
{
    IntersectionData d;
    d.object = 0;

    Path p;

    // Initialize the ray by sampling a light
    if (!initRay(s, d.ray, chooseLight(s)))
        return;

    // Look for a large but bounded number of bounces
    for (unsigned bounces = maxBounce; bounces; --bounces) {

        // Intersect with an object or the edge of the viewport
        bool hit = rayIntersect(d, s);

        // Append to Path
        p.append(d.point);

        if (!hit) {
            // Ray exited the scene after this.
            break;
        }

        if (!rayMaterial(d, s)) {
            // Ray was absorbed by material
            break;
        }
    }
    return p;
}

bool ZTrace::initRay(Sampler &s, Ray &r, const Value &light)
{
    double cartesianX = s.value(light[1]);
    double cartesianY = s.value(light[2]);
    double polarAngle = s.value(light[3]) * (M_PI / 180.0);
    double polarDistance = s.value(light[4]);
    r.origin.x = cartesianX + cos(polarAngle) * polarDistance;
    r.origin.y = cartesianY + sin(polarAngle) * polarDistance;

    double rayAngle = s.value(light[5]) * (M_PI / 180.0);
    r.setAngle(rayAngle);

    /*
     * Try to discard rays for invisible wavelengths without actually
     * counting them as real rays. (If we count them without tracing them,
     * our scene will render as unexpectedly dark since some of the
     * photons will not be visible on the rendered image.)
     */

    unsigned tries = 1000;
    for (;;) {
        double wavelength = s.value(light[6]);
        r.color.setWavelength(wavelength);
        if (r.color.isVisible()) {
            // Success
            break;
        } else {
            // Can we keep looking?
            if (!--tries)
                return false;
        }
    }

    return true;
}



bool ZTrace::rayIntersect(IntersectionData &d, Sampler &s)
{
    /*
     * Sample all objects in the scene that d.ray might hit. If we hit an object,
     * updates 'd' and returns true. If we don't, d.point will be clipped to the
     * edge of the image by rayIntersectBounds() and we return 'false'.
     */

    if (mQuadtree.rayIntersect(d, s)) {
        // Quadtree found an intersection
        return true;
    }

    // Nothing. Intersect at infinity.
    rayIntersectInf(d);
    return false;
}
void ZTrace::rayIntersectInf(IntersectionData &d) const {

    if (d.point.x < d.point.y? d.point.x : d.point.y) {
        double k = FLT_MAX/d.point.y;
        d.point.x = k*d.point.x;
        d.point.y = FLT_MAX;
    } else {
        double k = FLT_MAX/d.point.x;
        d.point.x = FLT_MAX;
        d.point.y = k*d.point.y;
    }
    d.normal.x = nan("NaN");
    d.normal.y = nan("NaN");

    d.distance = nan("NaN");
}

bool ZTrace::rayMaterial(IntersectionData &d, Sampler &s)
{
    /*
     * Sample the material indicated by the intersection 'd' and update the ray.
     * Returns 'true' if the ray continues propagating, or 'false' if it's abosrbed.
     */

    // Lookup in our material database
    const Value &object = *d.object;
    unsigned id = object[0u].GetUint();
    const Value &material = mMaterials[id];

    double r = s.uniform();
    double sum = 0;

    // Loop over all material outcomes, pick one according to our random variable.
    for (unsigned i = 0, e = material.Size(); i != e; ++i) {
        const Value& outcome = material[i];
        sum += outcome[0u].GetDouble();
        if (r <= sum)
            return ZMaterial::rayOutcome(outcome, d, s);
    }

    // Absorbed
    return false;
}


