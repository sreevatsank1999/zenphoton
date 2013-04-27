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
#include <vector>
#include <algorithm>


class HistogramImage
{
public:
    void resize(unsigned w, unsigned h);
    void clear();
    void render(std::vector<unsigned char> &rgb, double scale, double exponent);
    void plot(unsigned wl, unsigned intensity, unsigned x, unsigned y);
    void line(unsigned wl, double x0, double y0, double x1, double y1);

    unsigned width() const { return mWidth; }
    unsigned height() const { return mHeight; }

    // Pack a floating point light wavelength into a 16-bit integer.
    static unsigned packWavelength(double nm)
    {
        // Special case for monochromatic white
        if (nm == 0)
            return kWavelengthWhite;

        // Invisible?
        if (nm < kFirstWavelength || nm > kLastWavelength)
            return kWavelengthBlack;

        return (nm - kFirstWavelength) * double(1 << kWavelengthFixedBits);
    }

private:
    // Tiled memory organization, to improve data locality
    static const unsigned kTileBits = 4;
    static const unsigned kTileWidth = 1 << kTileBits;
    static const unsigned kTileMask = kTileWidth - 1;
    static const unsigned kPixelsPerTile = kTileWidth * kTileWidth;
    static const unsigned kSamplesPerTile = kPixelsPerTile * 3;
    static const unsigned kBufferLength = 0x4000;

    // Integer representation of wavelengths
    static const double kFirstWavelength = 360.0;
    static const double kLastWavelength = 830.0;
    static const unsigned kWavelengthFixedBits = 6;
    static const unsigned kWavelengthBlack = 0xFFFF;
    static const unsigned kWavelengthWhite = 0xFFFE;

    unsigned tilesWide() const { return (mWidth + kTileWidth - 1) >> kTileBits; };
    unsigned tilesHigh() const { return (mHeight + kTileWidth - 1) >> kTileBits; };

    // Actual requested width/height of image
    uint32_t mWidth, mHeight;

    // Main storage for each tile, organized by (y, x, channel).
    struct TileSamples {
        int64_t samples[kSamplesPerTile];
    };

    // Intermediate sorting buffer for each tile.
    // Stores a 16-bit color, 8-bit intensity, and 8-bit pixel number.
    struct TileBuffer {
        uint32_t records[kBufferLength];
    };

    std::vector<TileSamples> mSamples;          // Final counts for each tile
    std::vector<TileBuffer> mBuffer;            // Sorting buffer for each tile
    std::vector<uint16_t> mBufferCounts;        // Fill levels for all sorting buffers

    void lPlot(unsigned wl, unsigned intensity, unsigned x, unsigned y, bool swap);
    void lineImpl(unsigned wl, double x0, double y0, double x1, double y1, bool swap);
    void flushBuffer(unsigned tileNumber);
};


inline void HistogramImage::resize(unsigned w, unsigned h)
{
    mWidth = w;
    mHeight = h;

    unsigned numTiles = tilesWide() * tilesHigh();

    mSamples.resize(numTiles);
    mBuffer.resize(numTiles);
    mBufferCounts.resize(numTiles);

    clear();
}

inline void HistogramImage::clear()
{
    memset(&mSamples[0], 0, mSamples.size() * sizeof mSamples[0]);
    memset(&mBufferCounts[0], 0, mBufferCounts.size() * sizeof mBufferCounts[0]);
}

inline void HistogramImage::render(std::vector<unsigned char> &rgb, double scale, double exponent)
{
    // Tone mapping from 64-bit-per-channel to 8-bit-per-channel, with dithering.

    PRNG rng;
    rng.seed(0);

    unsigned rgbIndex = 0;
    rgb.resize(mWidth * mHeight * 3);

    for (unsigned i = 0, e = (unsigned)mBuffer.size(); i != e; ++i) {
        flushBuffer(i);
        mBufferCounts[i] = 0;
    }

    for (unsigned y = 0; y < mHeight; ++y)
        for (unsigned x = 0; x < mWidth; ++x)
            for (unsigned c = 0; c < 3; ++c) {

                unsigned tileX = x >> kTileBits;
                unsigned tileY = y >> kTileBits;
                unsigned tileNumber = tileX + tileY * tilesWide();
                unsigned pixelX = x & kTileMask;
                unsigned pixelY = y & kTileMask;
                unsigned sampleNumber = c + 3 * (pixelX + (pixelY << kTileBits));
                int64_t sample = mSamples[tileNumber].samples[sampleNumber];

                double v = pow(sample * scale, exponent) + 0.5 + rng.uniform(0, 0.5);

                rgb[rgbIndex++] = std::max(0.0, std::min(255.9, v));
            }
}

inline void HistogramImage::flushBuffer(unsigned tileNumber)
{
    /*
     * Write back all buffered records for a single tile. Does not update the
     * mBufferCounts for this tile; the caller is responsible for zeroing it,
     * since in plot() we can save a write.
     */

    unsigned count = mBufferCounts[tileNumber];
    uint32_t *ptr = mBuffer[tileNumber].records;
    TileSamples &ts = mSamples[tileNumber];

    while (count--) {
        uint32_t record = *(ptr++);

        unsigned pixelNumber = record >> 24;
        unsigned i = pixelNumber * 3;

        // Encoded wavelength and 8-bit intensity
        unsigned wl = record & 0xFFFF;
        unsigned intensity = (record >> 16) & 0xFF;

        // XXX: Wavelength to sRGB
        (void)wl;
        int r = intensity;
        int g = intensity;
        int b = intensity;

        ts.samples[i++] += r;
        ts.samples[i++] += g;
        ts.samples[i  ] += b;
    }
}

inline void __attribute__((always_inline)) HistogramImage::plot(unsigned wl, unsigned intensity, unsigned x, unsigned y)
{
    if (x >= mWidth || y >= mHeight)
        return;

    unsigned tileX = x >> kTileBits;
    unsigned tileY = y >> kTileBits;
    unsigned tileNumber = tileX + tileY * tilesWide();

    unsigned pixelX = x & kTileMask;
    unsigned pixelY = y & kTileMask;
    unsigned pixelNumber = pixelX + (pixelY << kTileBits);

    unsigned count = mBufferCounts[tileNumber];
    if (count == kBufferLength) {
        flushBuffer(tileNumber);
        count = 0;
    }

    // These writes tend to be sequential. Good for cache performance!
    mBuffer[tileNumber].records[count] = wl | (intensity << 16) | (pixelNumber << 24);
    mBufferCounts[tileNumber] = count + 1;
}

inline void __attribute__((always_inline)) HistogramImage::lPlot(unsigned wl, unsigned intensity, unsigned x, unsigned y, bool swap)
{
    // Plotter function used by line(). Capable of swapping X and Y axes based on (known at compile-time) 'swap' param.

    if (swap)
        plot(wl, intensity, y, x);
    else
        plot(wl, intensity, x, y);
}

inline void __attribute((always_inline)) HistogramImage::lineImpl(unsigned wl, double x0, double y0, double x1, double y1, bool swap)
{
    /*
     * Internal axis-swappable implementation of line().
     * "x" and "y" are only really "x" and "y" when swap==false. If swap==true, we're operating
     * on a rotated coordinate system which will be unrotated by lPlot().
     */

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    double dx = x1 - x0;
    double dy = y1 - y0;
    double gradient = dy / dx;
    double br = 128.0 * sqrt(dx*dx + dy*dy) / dx;

    // First endpoint
    double x05 = x0 + 0.5;
    int xpxl1 = x05;
    double xend = xpxl1;
    double yend = y0 + gradient * (xend - x0);
    double xgap = br * (1.0 - x05 + xend);
    int ypxl1 = yend;
    double t = yend - int(yend);
    lPlot(wl, xgap * (1.0 - t), xpxl1, ypxl1, swap);
    lPlot(wl, xgap * t, xpxl1, ypxl1 + 1, swap);
    double intery = yend + gradient;

    // Second endpoint
    double x15 = x1 + 0.5;
    int xpxl2 = x15;
    t = xpxl2;
    yend = y1 + gradient * (t - x1);
    xgap = br * (x15 - t);
    int ypxl2 = yend;
    t = yend - int(yend);
    lPlot(wl, xgap * (1.0 - t), xpxl2, ypxl2, swap);
    lPlot(wl, xgap * t, xpxl2, ypxl2 + 1, swap);

    // Inner loop
    while (++xpxl1 < xpxl2) {
        unsigned iy = intery;
        double fy = intery - iy;
        lPlot(wl, br * (1.0 - fy), xpxl1, iy, swap);
        lPlot(wl, br * fy, xpxl1, iy + 1, swap);
        intery += gradient;
    }
}

inline void HistogramImage::line(unsigned wl, double x0, double y0, double x1, double y1)
{
    /*
     * Modified version of Xiaolin Wu's antialiased line algorithm:
     * http://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
     *
     * Brightness compensation:
     *   The total brightness of the line should be proportional to its
     *   length, but with Wu's algorithm it's proportional to dx.
     *   We scale the brightness of each pixel to compensate.
     */

    double dx = x1 - x0;
    double dy = y1 - y0;
    if (dx < 0.0) dx = -dx;
    if (dy < 0.0) dy = -dy;

    if (dy > dx)
        lineImpl(wl, y0, x0, y1, x1, true);
    else   
        lineImpl(wl, x0, y0, x1, y1, false);
}
