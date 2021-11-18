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
#include <time.h>
#include "zrender.h"
#include "zmaterial.h"
#include "zcheck.h"

ZRender::ZRender(const Value &scene)
    : mScene(scene),
    mViewport(scene["viewport"]),
    ptrTracer(new ZTrace(scene))
{
    // Optional integer values
    mDebug = ZCheck::checkInteger(mScene["debug"], "debug");
    batchsize = 100000;

    // Check stopping conditions
    mRayLimit = ZCheck::ZCheck::checkNumber(scene["rays"], "rays");
    mTimeLimit = ZCheck::ZCheck::checkNumber(scene["timelimit"], "timelimit");
    ZCheck::checkStopCondition(mRayLimit,mTimeLimit);

    // Integer resolution values
    const Value& resolution = mScene["resolution"];
    if (ZCheck::checkTuple(resolution, "resolution", 2)) {
        mImage.resize(ZCheck::checkInteger(resolution[0u], "resolution[0]"),
                      ZCheck::checkInteger(resolution[1], "resolution[1]"));
    }

    // Other cached tuples
    ZCheck::checkTuple(mViewport, "viewport", 4);

}


void ZRender::render(std::vector<unsigned char> &pixels)
{

    /*
     * Debug flags
     */
#if DEBUG == 1
    if (mDebug & kDebugQuadtree) {
        ZQuadtree& Quadtree = ptrTracer->getZQuadTree();
        ZQuadtree::Visitor v = ZQuadtree::Visitor::root(&Quadtree);
        renderDebugQuadtree(v);
    }
#endif

    // Establish fixed viewport
    ViewportSample v;
    initViewport(v);

    /*
     * Keep tracing rays until a stopping condition is hit.
     * Returns the total number of rays traced.
     */
    uint64_t numRays = 0;
    double startTime = (double)time(0);
    std::vector<Path> traces;   traces.reserve(batchsize);
    while (1) {

        if (mRayLimit) {
            if (numRays > mRayLimit)
                break;
        }

        if (mTimeLimit) {
            // Check time limit
            double now = (double)time(0);
            if (now > startTime + mTimeLimit)
                break;
        }

        ptrTracer->traceRays(traces, batchsize);
        draw(traces,v);


        numRays += batchsize;
        traces.clear();
    }

    /*
     * Optional gamma correction. Defaults to linear, for compatibility with zenphoton.
     */

    double gamma = ZCheck::checkNumber(mScene["gamma"], "gamma");
    if (gamma <= 0.0)
        gamma = 1.0;

    /* 
     * Exposure calculation as a backward-compatible generalization of zenphoton.com.
     * We need to correct for differences due to resolution and due to the higher
     * fixed-point resolution we use during histogram rendering.
     */

    double exposure = mScene["exposure"].GetDouble();
    double areaScale = sqrt(double(width()) * height() / (1024 * 576));
    double intensityScale = ptrTracer->getLightPower() / (255.0 * 8192.0);
    double scale = exp(1.0 + 10.0 * exposure) * areaScale * intensityScale / numRays;

    mImage.render(pixels, scale, 1.0 / gamma);
}

void ZRender::draw(std::vector<Path> &Ps, ViewportSample &v){

    double w = width();
    double h = height();


    for(auto& trace : Ps){
        Vec2 p_1 = trace.get_origin();
        Color path_color; path_color.setWavelength(trace.get_wavelength());
        if(!path_color.isVisible())  continue;

        const int n = trace.size();
        for(int i=0; i<n-1; i++){
            // Draw a line from d.ray.origin to d.point
            mImage.line( path_color,
                v.xScale(p_1.x, w),
                v.yScale(p_1.y, h),
                v.xScale(trace[i].x, w),
                v.yScale(trace[i].y, h));

            p_1 = trace[i];
        }
        
        Vec2 last_point = trace[n-1];
        if(!is_insideViewport(trace[n-1],v)){
            // Setting up final ray to clip it to within the viewport
            Ray r;  r.origin = p_1;   
            r.direction.x = trace[n-1].x - p_1.x;
            r.direction.y = trace[n-1].y - p_1.y;
            r.slope = r.direction.y/r.direction.x;

            last_point = rayIntersectViewport(r,v);
        }
        
        // Draw last line 
            mImage.line( path_color,
                v.xScale(p_1.x, w),
                v.yScale(p_1.y, h),
                v.xScale(last_point.x, w),
                v.yScale(last_point.y, h));
    }
    

}

void ZRender::interrupt()
{
    /*
     * Interrupt traceRays() in progress. The render will return as
     * soon as the next batch of rays finishes and the histogram is rendered.
     */

    mRayLimit = -1;
}

void ZRender::initViewport(ViewportSample &v)
{
    // Sample the viewport. We do this once per ray.

    v.origin.x = ZCheck::checkNumber(mViewport[0u],"viewport[0]");
    v.origin.y = ZCheck::checkNumber(mViewport[1],"viewport[1]");
    v.size.x = ZCheck::checkInteger(mViewport[2],"viewport[2]");
    v.size.y = ZCheck::checkInteger(mViewport[3],"viewport[3]");
}

bool ZRender::is_insideViewport(Vec2 point, const ViewportSample &v){
    AABB viewportBox = {
        v.origin.x,
        v.origin.y,
        v.origin.x + v.size.x,
        v.origin.y + v.size.y
    };
    return ( viewportBox.left<point.x && point.x<viewportBox.right ) && ( viewportBox.top<point.y && point.y<viewportBox.bottom );
}

Vec2 ZRender::rayIntersectViewport(Ray &r, const ViewportSample &v)
{
    AABB viewportBox = {
        v.origin.x,
        v.origin.y,
        v.origin.x + v.size.x,
        v.origin.y + v.size.y
    };

    Vec2 point = r.pointAtDistance(r.intersectFurthestAABB(viewportBox));
    return point;
}

void ZRender::renderDebugQuadtree(ZQuadtree::Visitor &v)
{
    /*
     * For debugging, draw an outline around each quadtree AABB.
     */

    ViewportSample vp;
    initViewport(vp);

    Color c = { 0x7FFFFFFF, 0x7FFFFFFF, 0 };
    double w = width();
    double h = height();

    double left   = vp.xScale(v.bounds.left,   w);
    double top    = vp.yScale(v.bounds.top,    h);
    double right  = vp.xScale(v.bounds.right,  w);
    double bottom = vp.yScale(v.bounds.bottom, h);

    mImage.line(c, left, top, right, top);
    mImage.line(c, right, top, right, bottom);
    mImage.line(c, right, bottom, left, bottom);
    mImage.line(c, left, bottom, left, top);

    ZQuadtree::Visitor first = v.first();
    if (first) renderDebugQuadtree(first);

    ZQuadtree::Visitor second = v.second();
    if (second) renderDebugQuadtree(second);
}
