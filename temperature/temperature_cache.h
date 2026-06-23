/*
 * temperature_cache.h
 * Material Color Utilities — C Implementation
 *
 * This file was automatically ported from the official Kotlin implementation
 * by Claude (Anthropic) as a faithful algorithmic translation into C.
 *
 * The algorithms and numeric constants in this file implement published
 * color-science specifications (CAM16, HCT, WCAG contrast, CIE L*a*b*).
 * Those mathematical formulas are not copyrightable facts.
 *
 * This C translation is released under the MIT License:
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef MCU_TEMPERATURE_TEMPERATURE_CACHE_H_
#define MCU_TEMPERATURE_TEMPERATURE_CACHE_H_

#include "../hct/hct.h"

/**
 * Design utilities using color temperature theory.
 *
 * Provides analogous colors, complementary color, and lazy caching for all
 * calculations. Must be freed with TemperatureCache_free() when done.
 *
 * All 361 HCT colors at integer hues 0-360 with the same chroma/tone as the
 * input are precomputed on first access and cached internally.
 */
typedef struct TemperatureCache TemperatureCache;

/**
 * Create a TemperatureCache for the given input color.
 * The caller must call TemperatureCache_free() when done.
 */
TemperatureCache *TemperatureCache_create(Hct input);

/**
 * Free a TemperatureCache created by TemperatureCache_create().
 */
void TemperatureCache_free(TemperatureCache *cache);

/**
 * A color that complements the input color aesthetically —
 * the "opposite temperature" color on the hue wheel.
 */
Hct TemperatureCache_complement(TemperatureCache *cache);

/**
 * 5 colors that pair well with the input color.
 * Equidistant in temperature and adjacent in hue.
 * Writes results into out_colors[5]. Returns 5.
 */
int TemperatureCache_getAnalogousColors(TemperatureCache *cache,
                                         Hct out_colors[5]);

/**
 * A set of `count` colors with differing hues, equidistant in temperature,
 * divided into `divisions` sections of the hue wheel.
 * out_colors must be at least `count` entries.
 * Returns the number of colors written.
 */
int TemperatureCache_getAnalogousColorsN(TemperatureCache *cache,
                                          int count,
                                          int divisions,
                                          Hct *out_colors);

/**
 * Temperature relative to all colors with the same chroma/tone (0.0–1.0).
 */
double TemperatureCache_getRelativeTemperature(TemperatureCache *cache,
                                                const Hct *hct);

/**
 * Raw cool-warm temperature of a color. Values below 0 are cool, above 0 warm.
 */
double TemperatureCache_rawTemperature(const Hct *hct);

#endif /* MCU_TEMPERATURE_TEMPERATURE_CACHE_H_ */
