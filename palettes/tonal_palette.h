/*
 * tonal_palette.h
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
#ifndef MCU_PALETTES_TONAL_PALETTE_H_
#define MCU_PALETTES_TONAL_PALETTE_H_

#include "../hct/hct.h"

/**
 * A palette of colors constant in hue and chroma, varying only in tone.
 *
 * The cache (101 entries for tones 0-100) is heap-allocated on first use and
 * freed by TonalPalette_free(). A value of -1 in the cache means uncached.
 */
typedef struct {
  double hue;      /**< HCT hue, 0-360 */
  double chroma;   /**< HCT chroma */
  Hct    key_color; /**< Representative color (first tone from T50 matching chroma) */
  int    cache[101]; /**< Cached ARGB for tones 0-100; -1 = not cached */
} TonalPalette;

/**
 * Create a TonalPalette from an ARGB integer.
 */
TonalPalette TonalPalette_fromInt(int argb);

/**
 * Create a TonalPalette from an HCT color.
 */
TonalPalette TonalPalette_fromHct(const Hct *hct);

/**
 * Create a TonalPalette from explicit hue and chroma values.
 */
TonalPalette TonalPalette_fromHueAndChroma(double hue, double chroma);

/**
 * Returns the ARGB color at the given tone (0-100) from the palette.
 * Results are cached inside the TonalPalette struct.
 */
int TonalPalette_tone(TonalPalette *palette, int tone);

/**
 * Returns the HCT color at the given tone (fractional) from the palette.
 */
Hct TonalPalette_getHct(TonalPalette *palette, double tone);

#endif /* MCU_PALETTES_TONAL_PALETTE_H_ */
