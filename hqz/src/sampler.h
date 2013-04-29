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
#include "prng.h"
#include "spectrum.h"


/*
 * Samplers stochastically sample a JSON value which may be a random
 * variable. 'v' may be any of the following JSON constructs:
 *
 *      1.0             A single number. Always returns this value.
 *      null            Synonymous with zero.
 *      [ 1.0, 5.0 ]    A list of exactly two numbers. Samples uniformly.
 *      others          Reserved for future definition. (Zero)
 *
 */

class Sampler {
private:
    double offset;
    PRNG mRandom;

public:
    typedef rapidjson::Value Value;

    Sampler(uint32_t seed) {
        mRandom.seed(seed);
    }

    double uniform() {
        return mRandom.uniform();
    }

    double uniform(double a, double b) {
        return mRandom.uniform(a, b);
    }

    double blackbody(double temperature) {
        return Color::blackbodyWavelength(temperature, uniform());
    }

    double value(const Value &v)
    {
        if (v.IsNumber()) {
            // Constant
            return v.GetDouble();
        }

        if (v.IsArray() && v.Size() == 2 && v[0u].IsNumber()) {
            // 2-tuples starting with a number

            if (v[1].IsNumber())
                return uniform(v[0u].GetDouble(), v[1].GetDouble());

            if (v[1].IsString() && v[1].GetStringLength() == 1 && v[1].GetString()[0] == 'K')
                return blackbody(v[0u].GetDouble());
        }

        // Unknown
        return 0;
    }
};
