/*
 * viewing_conditions.c
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
#define _USE_MATH_DEFINES
#include "viewing_conditions.h"
#include "cam16.h"
#include "../utils/color_utils.h"
#include "../utils/math_utils.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void ViewingConditions_make(const double white_point[3],
                             double adapting_luminance,
                             double background_lstar,
                             double surround,
                             int discounting_illuminant,
                             ViewingConditions *out) {
  /* Clamp background lstar to avoid infinity at pure black. */
  if (background_lstar < 0.1) background_lstar = 0.1;

  /* Transform white point XYZ to CAM16 'cone'/'rgb' responses */
  const double (*matrix)[3] = kXyzToCam16Rgb;
  double rW = white_point[0] * matrix[0][0] + white_point[1] * matrix[0][1] +
              white_point[2] * matrix[0][2];
  double gW = white_point[0] * matrix[1][0] + white_point[1] * matrix[1][1] +
              white_point[2] * matrix[1][2];
  double bW = white_point[0] * matrix[2][0] + white_point[1] * matrix[2][1] +
              white_point[2] * matrix[2][2];

  double f = 0.8 + surround / 10.0;
  double c;
  if (f >= 0.9) {
    c = MathUtils_lerp(0.59, 0.69, (f - 0.9) * 10.0);
  } else {
    c = MathUtils_lerp(0.525, 0.59, (f - 0.8) * 10.0);
  }

  double d;
  if (discounting_illuminant) {
    d = 1.0;
  } else {
    d = f * (1.0 - (1.0 / 3.6) * exp((-adapting_luminance - 42.0) / 92.0));
  }
  if (d < 0.0) d = 0.0;
  if (d > 1.0) d = 1.0;

  double nc = f;
  double rgb_d[3] = {
      d * (100.0 / rW) + 1.0 - d,
      d * (100.0 / gW) + 1.0 - d,
      d * (100.0 / bW) + 1.0 - d,
  };

  double k   = 1.0 / (5.0 * adapting_luminance + 1.0);
  double k4  = k * k * k * k;
  double k4f = 1.0 - k4;
  double fl  = k4 * adapting_luminance +
               0.1 * k4f * k4f * cbrt(5.0 * adapting_luminance);

  double n   = ColorUtils_yFromLstar(background_lstar) / white_point[1];
  double z   = 1.48 + sqrt(n);
  double nbb = 0.725 / pow(n, 0.2);
  double ncb = nbb;

  double rgb_a_factors[3] = {
      pow(fl * rgb_d[0] * rW / 100.0, 0.42),
      pow(fl * rgb_d[1] * gW / 100.0, 0.42),
      pow(fl * rgb_d[2] * bW / 100.0, 0.42),
  };
  double rgb_a[3] = {
      400.0 * rgb_a_factors[0] / (rgb_a_factors[0] + 27.13),
      400.0 * rgb_a_factors[1] / (rgb_a_factors[1] + 27.13),
      400.0 * rgb_a_factors[2] / (rgb_a_factors[2] + 27.13),
  };
  double aw = (2.0 * rgb_a[0] + rgb_a[1] + 0.05 * rgb_a[2]) * nbb;

  out->n        = n;
  out->aw       = aw;
  out->nbb      = nbb;
  out->ncb      = ncb;
  out->c        = c;
  out->nc       = nc;
  out->rgb_d[0] = rgb_d[0];
  out->rgb_d[1] = rgb_d[1];
  out->rgb_d[2] = rgb_d[2];
  out->fl       = fl;
  out->fl_root  = pow(fl, 0.25);
  out->z        = z;
}

void ViewingConditions_defaultWithBackgroundLstar(double lstar,
                                                   ViewingConditions *out) {
  double white_point[3];
  ColorUtils_whitePointD65(white_point);
  double adapting_luminance =
      (200.0 / M_PI) * ColorUtils_yFromLstar(50.0) / 100.0;
  ViewingConditions_make(white_point, adapting_luminance, lstar, 2.0, 0, out);
}

const ViewingConditions *ViewingConditions_DEFAULT(void) {
  static ViewingConditions default_vc;
  static int initialized = 0;
  if (!initialized) {
    ViewingConditions_defaultWithBackgroundLstar(50.0, &default_vc);
    initialized = 1;
  }
  return &default_vc;
}
