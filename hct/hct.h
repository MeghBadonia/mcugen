/*
 * hct.h
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
#ifndef MCU_HCT_HCT_H_
#define MCU_HCT_HCT_H_

#include "viewing_conditions.h"

/**
 * HCT color: Hue, Chroma, Tone.
 *
 * A color system built from CAM16 hue and chroma, and L* (tone).
 * Unlike Y in XYZ, L* is linear to human perception.
 */
typedef struct {
  double hue;    /**< 0 <= hue < 360 */
  double chroma; /**< 0 <= chroma < ~130 for sRGB */
  double tone;   /**< 0 <= tone <= 100 (L* in L*a*b*) */
  int    argb;   /**< ARGB integer representation */
} Hct;

/**
 * Create an HCT color from hue, chroma, and tone.
 * Invalid hue/tone values are corrected; chroma may be lowered to gamut limit.
 */
Hct Hct_from(double hue, double chroma, double tone);

/**
 * Create an HCT color from an ARGB integer.
 */
Hct Hct_fromInt(int argb);

/**
 * Return the ARGB integer for this HCT color.
 */
int Hct_toInt(const Hct *hct);

/**
 * Set hue; chroma may decrease. Returns updated Hct.
 */
Hct Hct_setHue(const Hct *hct, double new_hue);

/**
 * Set chroma; chroma may decrease. Returns updated Hct.
 */
Hct Hct_setChroma(const Hct *hct, double new_chroma);

/**
 * Set tone. Returns updated Hct.
 */
Hct Hct_setTone(const Hct *hct, double new_tone);

/**
 * Translate a color into different ViewingConditions.
 */
Hct Hct_inViewingConditions(const Hct *hct, const ViewingConditions *vc);

/** Returns 1 if hue is in the "blue" range (250-270). */
int Hct_isBlue(double hue);

/** Returns 1 if hue is in the "yellow" range (105-125). */
int Hct_isYellow(double hue);

/** Returns 1 if hue is in the "cyan" range (170-207). */
int Hct_isCyan(double hue);

#endif /* MCU_HCT_HCT_H_ */
