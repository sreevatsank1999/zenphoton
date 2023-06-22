
#include <float.h>
#include "hqz/ztrace.h"
#include "hqz/zcheck.h"
#include "hqz/zmaterial.h"
#include <iostream>
#include <chrono>

ZTrace::ZTrace(const Value &scene)
    : mLights(scene["lights"]),
    mObjects(scene["objects"]),
    mMaterials(scene["materials"]),
    mLightPower(0.0),
    mSeed(time(0) % (static_cast<time_t>(1)<<8*sizeof(mSeed))), maxReflection(1000)
#if ENABLE_PARALLEL == 1
    , useParallel(ZCheck::checkBool(scene["parallel"],"parallel"))
#endif
{
    // Optional integer values
    if(scene.HasMember("seed"))
        mSeed = ZCheck::checkInteger(scene["seed"], "seed");

    if(scene.HasMember("maxReflection"))
        maxReflection = ZCheck::checkInteger(scene["maxReflection"], "maxReflection");      

    // Add up the total light power in the scene, and check all lights.
    if (ZCheck::checkTuple(mLights, "viewport", 1)) {
        for (unsigned i = 0; i < mLights.Size(); ++i) {
            const Value &light = mLights[i];
            if (ZCheck::checkTuple(light, "light", 7))
                mLightPower += light[0u].GetDouble();
        }
    }
    ZCheck::checkLightPower(mLightPower);

    // Check all objects
    if (ZCheck::checkTuple(mObjects, "objects", 0)) {
        for (unsigned i = 0; i < mObjects.Size(); ++i) {
            const Value &object = mObjects[i];
            if (ZCheck::checkTuple(object, "object", 5)) {
                ZCheck::checkMaterialID(object[0u],mMaterials);
            }
        }
    }

    // Check all materials
    if (ZCheck::checkTuple(mMaterials, "materials", 0)) {
        for (unsigned i = 0; i < mMaterials.Size(); ++i)
            ZCheck::checkMaterialValue(i,mMaterials);
    }

    mQuadtree.build(mObjects);
}

#if DEBUG == 1
ZQuadtree& ZTrace::getZQuadTree() {
    return mQuadtree;
}
#endif

double ZTrace::getLightPower() const {
    return mLightPower;
}

void ZTrace::traceRays(Paths &paths, uint32_t nbRays){

#if ENABLE_PARALLEL == 1
    if(useParallel){

        const auto start = std::chrono::steady_clock::now();
        __traceRays_parallel(paths, nbRays);
        const auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Parallel Trace Time taken: " << elapsed.count() << "(nbRays:" << nbRays << ")\n";
    }
    else{

        const auto start = std::chrono::steady_clock::now();
        __traceRays(paths, nbRays);
        const auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Serial Trace Time taken: " << elapsed.count() << "(nbRays:" << nbRays << ")\n";
    }
#else
    __traceRays(paths, nbRays);
#endif
}

void ZTrace::__traceRays(Paths &paths, uint32_t nbRays)
{
    paths.resize(nbRays);
    
    for(uint64_t ray_id=0; ray_id<nbRays; ray_id++)
        paths[ray_id] = traceRay(Sampler(mSeed + ray_id));

    mSeed += nbRays;
}

#if ENABLE_PARALLEL == 1
void ZTrace::__traceRays_parallel(Paths &paths, uint32_t nbRays)
{
    paths.resize(nbRays);
    
    #pragma omp parallel for schedule(static)
    for(uint64_t ray_id=0; ray_id<nbRays; ray_id++)
        paths[ray_id] = traceRay(Sampler(mSeed + ray_id));
    
    mSeed += nbRays;
}
#endif

Path ZTrace::traceRay(Sampler&& s)
{
    IntersectionData d;
    d.object = 0;

    // Initialize the ray by sampling a light
    double lambda;
    initRay(s, d.ray, lambda, chooseLight(s));
     
    Path p(d.ray.origin,lambda);

    // Look for a large but bounded number of bounces
    for (unsigned bounces = maxReflection; bounces; --bounces) {

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

void ZTrace::initRay(Sampler &s, Ray &r, double &wavelength, const Value &light)
{
    double cartesianX = s.value(light[1]);
    double cartesianY = s.value(light[2]);
    double polarAngle = s.value(light[3]) * (M_PI / 180.0);
    double polarDistance = s.value(light[4]);
    r.origin = Vec2(cartesianX,cartesianY) + Vec2(cos(polarAngle),sin(polarAngle)) * polarDistance;

    double rayAngle = s.value(light[5]) * (M_PI / 180.0);
    r.setAngle(rayAngle);

    wavelength = s.value(light[6]);
    r.color.setWavelength(wavelength);

}

const ZTrace::Value& ZTrace::chooseLight(Sampler &s){

    // Pick a random light, using the light power as a probability weight.
    // Fast path for scenes with only one light.

    unsigned i = 0;
    unsigned last = mLights.Size() - 1;

    if (i != last) {
        double r = s.uniform(0, mLightPower);
        double sum = 0;

        // Check all lights except the last
        do {
            const Value& light = mLights[i++];
            sum += s.value(light[0u]);
            if (r <= sum)
                return light;
        } while (i != last);
    }

    // Default, last light.
    return mLights[last];
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
    
    /*
     * This ray is never going to hit an object. Update d.point with the furthest away
     * InfBox intersection, if any. If this ray never intersects the InfBox, d.point
     * will equal d.ray.origin.
     */

        AABB Inf = {
        -FLT_MAX,
        -FLT_MAX,
        FLT_MAX,
        FLT_MAX
    };

    d.point = d.ray.pointAtDistance(d.ray.intersectFurthestAABB(Inf));

    d.normal = Vec2(nan("NaN"),nan("NaN"));

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


