/*
 * contrast.c
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
#include "contrast.h"
#include "../utils/color_utils.h"
#include <math.h>

#define CONTRAST_RATIO_EPSILON     0.04
#define LUMINANCE_GAMUT_MAP_TOLERANCE 0.4

double Contrast_ratioOfYs(double y1, double y2) {
  double lighter = (y1 > y2) ? y1 : y2;
  double darker  = (lighter == y2) ? y1 : y2;
  return (lighter + 5.0) / (darker + 5.0);
}

double Contrast_ratioOfTones(double t1, double t2) {
  return Contrast_ratioOfYs(ColorUtils_yFromLstar(t1),
                              ColorUtils_yFromLstar(t2));
}

int Contrast_lighter(double tone, double ratio, double *out) {
  if (tone < 0.0 || tone > 100.0) return -1;
  double dark_y  = ColorUtils_yFromLstar(tone);
  double light_y = ratio * (dark_y + 5.0) - 5.0;
  if (light_y < 0.0 || light_y > 100.0) return -1;
  double real_contrast = Contrast_ratioOfYs(light_y, dark_y);
  double delta = fabs(real_contrast - ratio);
  if (real_contrast < ratio && delta > CONTRAST_RATIO_EPSILON) return -1;
  double return_value =
      ColorUtils_lstarFromY(light_y) + LUMINANCE_GAMUT_MAP_TOLERANCE;
  if (return_value < 0.0 || return_value > 100.0) return -1;
  *out = return_value;
  return 0;
}

double Contrast_lighterUnsafe(double tone, double ratio) {
  double result;
  return (Contrast_lighter(tone, ratio, &result) == 0) ? result : 100.0;
}

int Contrast_darker(double tone, double ratio, double *out) {
  if (tone < 0.0 || tone > 100.0) return -1;
  double light_y = ColorUtils_yFromLstar(tone);
  double dark_y  = (light_y + 5.0) / ratio - 5.0;
  if (dark_y < 0.0 || dark_y > 100.0) return -1;
  double real_contrast = Contrast_ratioOfYs(light_y, dark_y);
  double delta = fabs(real_contrast - ratio);
  if (real_contrast < ratio && delta > CONTRAST_RATIO_EPSILON) return -1;
  double return_value =
      ColorUtils_lstarFromY(dark_y) - LUMINANCE_GAMUT_MAP_TOLERANCE;
  if (return_value < 0.0 || return_value > 100.0) return -1;
  *out = return_value;
  return 0;
}

double Contrast_darkerUnsafe(double tone, double ratio) {
  double result;
  return (Contrast_darker(tone, ratio, &result) == 0) ? result : 0.0;
}
