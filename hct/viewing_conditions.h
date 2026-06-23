/*
 * viewing_conditions.h
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
#ifndef MCU_HCT_VIEWING_CONDITIONS_H_
#define MCU_HCT_VIEWING_CONDITIONS_H_

/**
 * In traditional color spaces, a color can be identified solely by the
 * observer's measurement of the color. Color appearance models such as CAM16
 * also use information about the environment where the color was observed,
 * known as the viewing conditions.
 *
 * This struct caches intermediate values of the CAM16 conversion process that
 * depend only on viewing conditions, enabling speed-ups.
 */
typedef struct {
  double n;
  double aw;
  double nbb;
  double ncb;
  double c;
  double nc;
  double rgb_d[3];
  double fl;
  double fl_root;
  double z;
} ViewingConditions;

/**
 * Create ViewingConditions from a physically relevant set of parameters.
 *
 * @param white_point   White point in XYZ. Use ColorUtils_whitePointD65() for
 *                      default (D65).
 * @param adapting_luminance  Luminance of the adapting field (lux * 0.0586).
 *                            Default ~11.72 (200 lux).
 * @param background_lstar    L* of the area surrounding the color.
 *                            Default 50.0.
 * @param surround      0=pitch dark, 1=dim room, 2=matching surroundings.
 *                      Default 2.0.
 * @param discounting_illuminant  Whether the eye adapts to illuminant tint.
 *                                Default 0 (false).
 * @param out           Output ViewingConditions struct.
 */
void ViewingConditions_make(const double white_point[3],
                             double adapting_luminance,
                             double background_lstar,
                             double surround,
                             int discounting_illuminant,
                             ViewingConditions *out);

/**
 * Create sRGB-like viewing conditions with a custom background lstar.
 */
void ViewingConditions_defaultWithBackgroundLstar(double lstar,
                                                   ViewingConditions *out);

/**
 * Returns a pointer to the default (sRGB-like, lstar=50) viewing conditions.
 * The returned pointer is valid for the lifetime of the process.
 */
const ViewingConditions *ViewingConditions_DEFAULT(void);

#endif /* MCU_HCT_VIEWING_CONDITIONS_H_ */
