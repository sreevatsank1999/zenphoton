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
#include "rapidjson/document.h"
#include "ray.h"
#include "sampler.h"
#include "zobject.h"
#include <cfloat>
#include <vector>


class ZQuadtree {
public:
    typedef rapidjson::Value Value;
    typedef std::vector<const Value*> ValueArray;

    void build(const Value &objects);
    bool rayIntersect(IntersectionData &d, Sampler &s);

private:
    struct Visitor;

    struct Node
    {
        ValueArray objects;     // Objects that don't fully fit in either child
        double split;           // Split location
        Node *children[2];      // [ < split, >= split ]
    };

    Node root;

    bool rayIntersect(IntersectionData &d, Sampler &s, Visitor &v);
};


struct ZQuadtree::Visitor
{
    Node *current;
    AABB bounds;
    bool axisY;

    operator bool () const {
        return current != 0;
    }

    static Visitor root(ZQuadtree *tree)
    {
        Visitor v;
        v.current = &tree->root;
        v.bounds.left = v.bounds.top = DBL_MIN;
        v.bounds.right = v.bounds.bottom = DBL_MAX;
        v.axisY = false;
        return v;
    }

    Visitor first()
    {
        Visitor v;

        v.current = current->children[0];
        v.axisY = !axisY;
        v.bounds = bounds;

        if (axisY)
            v.bounds.bottom = current->split;
        else
            v.bounds.right = current->split;

        return v;
    }

    Visitor second()
    {
        Visitor v;

        v.current = current->children[1];
        v.axisY = !axisY;
        v.bounds = bounds;

        if (axisY)
            v.bounds.top = current->split;
        else
            v.bounds.left = current->split;

        return v;
    }
};


inline void ZQuadtree::build(const Value &objects)
{
    // XXX: Putting all objects in the root node for now

    root.split = 0;
    root.children[0] = 0;
    root.children[1] = 0;

    root.objects.resize(objects.Size());

    for (unsigned i = 0; i < objects.Size(); ++i)
        root.objects[i] = &objects[i];
}

inline bool ZQuadtree::rayIntersect(IntersectionData &d, Sampler &s)
{
    Visitor v = Visitor::root(this);
    return rayIntersect(d, s, v);
}

inline bool ZQuadtree::rayIntersect(IntersectionData &d, Sampler &s, Visitor &v)
{
    // Iterate over some objects, keeping track of the closest intersection
    IntersectionData temp = d;
    IntersectionData closest;
    closest.distance = DBL_MAX;
    bool result = false;

    Visitor first = v.first();
    double firstClosest, firstFurthest;
    bool firstHit = first && d.ray.intersectAABB(first.bounds, firstClosest, firstFurthest);

    Visitor second = v.second();
    double secondClosest, secondFurthest;
    bool secondHit = second && d.ray.intersectAABB(second.bounds, secondClosest, secondFurthest);

    // Try the closest child first. Maybe we can skip testing the other side.
    if (firstClosest < secondClosest) {

        if (firstHit && firstClosest < closest.distance &&
            rayIntersect(temp, s, first) && temp.distance < closest.distance) {
            closest = temp;
            result = true;
        }

        if (secondHit && secondClosest < closest.distance &&
            rayIntersect(temp, s, second) && temp.distance < closest.distance) {
            closest = temp;
            result = true;
        }

    } else {

        if (secondHit && secondClosest < closest.distance &&
            rayIntersect(temp, s, second) && temp.distance < closest.distance) {
            closest = temp;
            result = true;
        }

        if (firstHit && firstClosest < closest.distance &&
            rayIntersect(temp, s, first) && temp.distance < closest.distance) {
            closest = temp;
            result = true;
        }
    }

    // Now try local objects. These could be leaves in the tree, or larger objects that
    // don't fully fit inside a subtree's AABB.

    Node &node = *v.current;
    for (ValueArray::const_iterator i = node.objects.begin(), e = node.objects.end(); i != e; ++i)
    { 
        const Value &object = **i;

        if (d.object != &object
            && ZObject::rayIntersect(object, temp, s)
            && temp.distance < closest.distance)
        {
            closest = temp;
            closest.object = &object;
            result = true;
        }
    }

    d = closest;
    return result;
}
