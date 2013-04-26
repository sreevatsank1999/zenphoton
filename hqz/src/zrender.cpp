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

#include "zrender.h"


ZRender::ZRender(const Value &scene)
    : mScene(scene),
    mViewport(scene["viewport"])
{
    // Seed the PRNG. (Zero by default)
    mRandom.seed(sampleValue(mScene["seed"]));

    // Integer resolution values
    const Value& resolution = mScene["resolution"];
    if (checkTuple(resolution, "resolution", 2))
        image.resize(sampleValue(resolution[0u]), sampleValue(resolution[1]));

    // Cached tuples
    checkTuple(mViewport, "viewport", 4);
}

void ZRender::render(std::vector<unsigned char> &pixels)
{
    for (unsigned i = 0; i < 200000; i++)
        image.line(127, 200, 127,
            mRandom.uniform(0, width()),
            mRandom.uniform(0, height()),
            mRandom.uniform(0, width()),
            mRandom.uniform(0, height()));

    image.render(pixels, 0.0005, 2.0);
}

bool ZRender::checkTuple(const Value &v, const char *noun, unsigned expected)
{
    /*
     * A quick way to check for a tuple with an expected length, and set
     * an error if there's any problem. Returns true on success.
     */

    if (!v.IsArray() || v.Size() < expected) {
        mError << "'" << noun << "' expected an array with "
            << expected << " item" << (expected == 1 ? "" : "s") << "\n";
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
