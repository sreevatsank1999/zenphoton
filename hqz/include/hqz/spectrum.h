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
#include <stdint.h>
#include <math.h>
#if ENABLE_PARALLEL == 1
    #include <omp.h>
#endif

struct Color
{
    int r, g, b;

    void setWavelength(double nm);

    static double blackbodyWavelength(double temperature, double uniform);
    static void testSpectrum(double temperature);

    void __attribute__((always_inline)) plot(int64_t *ptr, int intensity)
    {   
        
    #if ENABLE_PARALLEL == 1
        const int64_t r_intensity = r * intensity;
        const int64_t g_intensity = g * intensity;
        const int64_t b_intensity = b * intensity;
        
        int64_t * const r_ptr = ptr+0;
        int64_t * const g_ptr = ptr+1;
        int64_t * const b_ptr = ptr+2;
        
        #pragma omp atomic
        *r_ptr += r_intensity;
        #pragma omp atomic
        *g_ptr += g_intensity;
        #pragma omp atomic
        *b_ptr += b_intensity;
    #else
        ptr[0] += r * intensity;
        ptr[1] += g * intensity;
        ptr[2] += b * intensity;
    #endif

    }

    bool isVisible()
    {
        return r || g || b;
    }
};
