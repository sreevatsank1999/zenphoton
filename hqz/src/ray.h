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

#pragma once
#include <math.h>
#include <float.h>
#include <algorithm>
#include "spectrum.h"


struct Vec2
{
    double x, y;
};

struct AABB
{
    double left, top, right, bottom;

    bool contains(const AABB &other)
    {
        return other.left >= left &&
               other.right <= right &&
               other.top >= top &&
               other.bottom <= bottom;
    }
};

struct Ray
{
    Vec2 origin;
    Vec2 direction;
    Color color;
    double slope;

    void setAngle(double r)
    {
        direction.x = cos(r);
        direction.y = sin(r);
        slope = direction.y / direction.x;
    }

    void reflect(Vec2 normal)
    {
        // Does *not* require 'normal' to already be normalized.

        double t = 2.0 * (normal.x * direction.x + normal.y * direction.y)
            / (normal.x * normal.x + normal.y * normal.y);
        direction.x -= t * normal.x;
        direction.y -= t * normal.y;
        slope = direction.y / direction.x;
    }

    Vec2 pointAtDistance(double distance) const
    {
        Vec2 result = { origin.x + distance * direction.x, origin.y + distance * direction.y };
        return result;
    }

    bool intersectSegment(Vec2 s1, Vec2 sD, double &distance, double &alpha) const
    {
        /*
         * Ray to Segment Intersection.
         * On intersection:
         *   - returns true
         *   - 'distance' is the distance from ray origin to intersection
         *   - 'alpha' is in the range [0,1], and represents how far along the segment we hit.
         */

        /*
         * Ray equation: [rayOrigin + rayDirection * M], 0 <= M
         * Segment equation: [p1 + (p2-p1) * N], 0 <= N <= 1
         * Returns true with dist=M if we find an intersection.
         *
         *  M = (seg1.x + segD.x * N - rayOrigin.x) / rayDirection.x
         *  M = (seg1.y + segD.y * N - rayOrigin.y) / rayDirection.y
         *
         * First solving for N, to see if there's an intersection at all:
         *
         *  M = (seg1.x + segD.x * N - rayOrigin.x) / rayDirection.x
         *  N = (M * rayDirection.y + rayOrigin.y - seg1.y) / segD.y
         *
         *  N = (((seg1.x + segD.x * N - rayOrigin.x) / rayDirection.x) *
         *     rayDirection.y + rayOrigin.y - seg1.y) / segD.y
         */

        double n = ((s1.x - origin.x) * slope + (origin.y - s1.y)) / (sD.y - sD.x * slope);
        if (n < 0.0) return false;
        if (n > 1.0) return false;

        // Now solve for M, the ray/segment distance

        double m = (s1.x + sD.x * n - origin.x) / direction.x;
        if (m < 0.0) return false;

        distance = m;
        alpha = n;
        return true;
    }

    bool intersectSegment(Vec2 s1, Vec2 sD, double &distance) const
    {
        double alpha;
        return intersectSegment(s1, sD, distance, alpha);
    }

    bool intersectAABB(const AABB &box, double &closest, double &furthest) const
    {
        /*
         * Ray to Axis-Aligned Bounding Box intersection.
         * Always updates 'closest' and 'furthest'. Returns 'true' if any intersection happened.
         */

        Vec2 topLeft = { box.left, box.top };
        Vec2 topRight = { box.right, box.top };
        Vec2 bottomLeft = { box.left, box.bottom };
        Vec2 horizontal = { box.right - box.left, 0 };
        Vec2 vertical = { 0, box.bottom - box.top };

        double dist;
        bool success = false;

        furthest = 0;
        closest = FLT_MAX;

        if (intersectSegment(topLeft, horizontal, dist)) {
            success = true;
            furthest = std::max(furthest, dist);
            closest = std::min(closest, dist);
        }
        if (intersectSegment(bottomLeft, horizontal, dist)) {
            success = true;
            furthest = std::max(furthest, dist);
            closest = std::min(closest, dist);
        }
        if (intersectSegment(topLeft, vertical, dist)) {
            success = true;
            furthest = std::max(furthest, dist);
            closest = std::min(closest, dist);
        }
        if (intersectSegment(topRight, vertical, dist)) {
            success = true;
            furthest = std::max(furthest, dist);
            closest = std::min(closest, dist);
        }

        return success;
    }
};

struct IntersectionData
{
    Ray ray;

    Vec2 point;
    Vec2 normal;
    double distance;

    // IN: Previous object, to exclude.  OUT: Object we hit.
    const rapidjson::Value *object;
};
