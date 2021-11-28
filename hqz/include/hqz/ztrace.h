
#pragma once
#include "rapidjson/document.h"
#include "sampler.h"
#include "zquadtree.h"
#include "ray.h"
#include "path.h"
#include <stdint.h>
#include <vector>
#if ENABLE_PARALLEL == 1
    #include <omp.h>
#endif

class ZTrace{
public:
    typedef rapidjson::Value Value;

    ZTrace(const Value &scene);

    void traceRays(Paths &paths, uint32_t nbRays);

#if DEBUG == 1
    // Access to internal Data structure for debugging
    ZQuadtree& getZQuadTree();
#endif

    double getLightPower() const;

private:
    static const uint32_t kDebugQuadtree = 1 << 0;

    ZQuadtree mQuadtree;

    const Value& mLights;
    const Value& mObjects;
    const Value& mMaterials;

    uint32_t mSeed;
    double mLightPower;

    uint32_t maxReflection;

#if ENABLE_PARALLEL == 1
    // Use Parellel?
    const bool useParallel;
#endif

    // Raytracer entry point
    void __traceRays(Paths &paths, uint32_t nbRays);
    Path traceRay(Sampler&& s);

#if ENABLE_PARALLEL == 1
    // Raytracer entry point (Parallel)
    void __traceRays_parallel(Paths &paths, uint32_t nbRays);
#endif

    // Light sampling
    const Value &chooseLight(Sampler &s);
    void initRay(Sampler &s, Ray &r, double &wavelength, const Value &light);

    // Material sampling
    bool rayMaterial(IntersectionData &d, Sampler &s);

    // Object sampling
    bool rayIntersect(IntersectionData &d, Sampler &s);
    void rayIntersectInf(IntersectionData &d) const;

};