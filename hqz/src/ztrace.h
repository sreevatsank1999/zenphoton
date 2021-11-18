
#pragma once
#include "rapidjson/document.h"
#include "sampler.h"
#include "zquadtree.h"
#include "ray.h"
#include "path.h"
#include <stdint.h>
#include <vector>

class ZTrace{
public:
    typedef rapidjson::Value Value;

    ZTrace(const Value &scene);

    void traceRays(std::vector<Path> &paths, uint32_t nbRays);

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

    uint32_t batchsize;
    uint32_t maxReflection;

    // Raytracer entry point
    Path traceRay(Sampler &s);
    std::vector<Path> traceRayBatch(uint32_t seed, uint32_t count);

    // Light sampling
    const Value &chooseLight(Sampler &s);
    bool initRay(Sampler &s, Ray &r, const Value &light);

    // Material sampling
    bool rayMaterial(IntersectionData &d, Sampler &s);

    // Object sampling
    bool rayIntersect(IntersectionData &d, Sampler &s);
    void rayIntersectInf(IntersectionData &d) const;

};