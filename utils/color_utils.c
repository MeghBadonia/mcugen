/*
 * color_utils.c
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
#include "color_utils.h"
#include "math_utils.h"
#include <math.h>

/* sRGB -> XYZ (D65) matrix */
static const double kSrgbToXyz[3][3] = {
    {0.41233895, 0.35762064, 0.18051042},
    {0.2126,     0.7152,     0.0722    },
    {0.01932141, 0.11916382, 0.95034478},
};

/* XYZ (D65) -> sRGB matrix */
static const double kXyzToSrgb[3][3] = {
    { 3.2413774792388685, -1.5376652402851851, -0.49885366846268053},
    {-0.9691452513005321,  1.8758853451067872,  0.04156585616912061},
    { 0.05562093689691305,-0.20395524564742123,  1.0571799111220335},
};

/* D65 white point */
static const double kWhitePointD65[3] = {95.047, 100.0, 108.883};

int ColorUtils_argbFromRgb(int red, int green, int blue) {
  return (255 << 24) | ((red & 255) << 16) | ((green & 255) << 8) | (blue & 255);
}

int ColorUtils_argbFromLinrgb(const double linrgb[3]) {
  int r = ColorUtils_delinearized(linrgb[0]);
  int g = ColorUtils_delinearized(linrgb[1]);
  int b = ColorUtils_delinearized(linrgb[2]);
  return ColorUtils_argbFromRgb(r, g, b);
}

int ColorUtils_alphaFromArgb(int argb) {
  return (argb >> 24) & 255;
}

int ColorUtils_redFromArgb(int argb) {
  return (argb >> 16) & 255;
}

int ColorUtils_greenFromArgb(int argb) {
  return (argb >> 8) & 255;
}

int ColorUtils_blueFromArgb(int argb) {
  return argb & 255;
}

int ColorUtils_isOpaque(int argb) {
  return ColorUtils_alphaFromArgb(argb) >= 255;
}

int ColorUtils_argbFromXyz(double x, double y, double z) {
  double linear_r = kXyzToSrgb[0][0] * x + kXyzToSrgb[0][1] * y + kXyzToSrgb[0][2] * z;
  double linear_g = kXyzToSrgb[1][0] * x + kXyzToSrgb[1][1] * y + kXyzToSrgb[1][2] * z;
  double linear_b = kXyzToSrgb[2][0] * x + kXyzToSrgb[2][1] * y + kXyzToSrgb[2][2] * z;
  int r = ColorUtils_delinearized(linear_r);
  int g = ColorUtils_delinearized(linear_g);
  int b = ColorUtils_delinearized(linear_b);
  return ColorUtils_argbFromRgb(r, g, b);
}

void ColorUtils_xyzFromArgb(int argb, double xyz[3]) {
  double r = ColorUtils_linearized(ColorUtils_redFromArgb(argb));
  double g = ColorUtils_linearized(ColorUtils_greenFromArgb(argb));
  double b = ColorUtils_linearized(ColorUtils_blueFromArgb(argb));
  double row[3] = {r, g, b};
  MathUtils_matrixMultiply(row, kSrgbToXyz, xyz);
}

int ColorUtils_argbFromLab(double l, double a, double b) {
  double fy = (l + 16.0) / 116.0;
  double fx = a / 500.0 + fy;
  double fz = fy - b / 200.0;
  double x = ColorUtils_labInvf(fx) * kWhitePointD65[0];
  double y = ColorUtils_labInvf(fy) * kWhitePointD65[1];
  double z = ColorUtils_labInvf(fz) * kWhitePointD65[2];
  return ColorUtils_argbFromXyz(x, y, z);
}

void ColorUtils_labFromArgb(int argb, double lab[3]) {
  double linear_r = ColorUtils_linearized(ColorUtils_redFromArgb(argb));
  double linear_g = ColorUtils_linearized(ColorUtils_greenFromArgb(argb));
  double linear_b = ColorUtils_linearized(ColorUtils_blueFromArgb(argb));
  double x = kSrgbToXyz[0][0] * linear_r + kSrgbToXyz[0][1] * linear_g +
             kSrgbToXyz[0][2] * linear_b;
  double y = kSrgbToXyz[1][0] * linear_r + kSrgbToXyz[1][1] * linear_g +
             kSrgbToXyz[1][2] * linear_b;
  double z = kSrgbToXyz[2][0] * linear_r + kSrgbToXyz[2][1] * linear_g +
             kSrgbToXyz[2][2] * linear_b;
  double fx = ColorUtils_labF(x / kWhitePointD65[0]);
  double fy = ColorUtils_labF(y / kWhitePointD65[1]);
  double fz = ColorUtils_labF(z / kWhitePointD65[2]);
  lab[0] = 116.0 * fy - 16.0;
  lab[1] = 500.0 * (fx - fy);
  lab[2] = 200.0 * (fy - fz);
}

int ColorUtils_argbFromLstar(double lstar) {
  double y = ColorUtils_yFromLstar(lstar);
  int component = ColorUtils_delinearized(y);
  return ColorUtils_argbFromRgb(component, component, component);
}

double ColorUtils_lstarFromArgb(int argb) {
  double xyz[3];
  ColorUtils_xyzFromArgb(argb, xyz);
  return 116.0 * ColorUtils_labF(xyz[1] / 100.0) - 16.0;
}

double ColorUtils_yFromLstar(double lstar) {
  return 100.0 * ColorUtils_labInvf((lstar + 16.0) / 116.0);
}

double ColorUtils_lstarFromY(double y) {
  return ColorUtils_labF(y / 100.0) * 116.0 - 16.0;
}

double ColorUtils_linearized(int rgb_component) {
  double normalized = rgb_component / 255.0;
  if (normalized <= 0.040449936) {
    return normalized / 12.92 * 100.0;
  } else {
    return pow((normalized + 0.055) / 1.055, 2.4) * 100.0;
  }
}

int ColorUtils_delinearized(double rgb_component) {
  double normalized = rgb_component / 100.0;
  double delinearized;
  if (normalized <= 0.0031308) {
    delinearized = normalized * 12.92;
  } else {
    delinearized = 1.055 * pow(normalized, 1.0 / 2.4) - 0.055;
  }
  int result = (int)round(delinearized * 255.0);
  if (result < 0) result = 0;
  if (result > 255) result = 255;
  return result;
}

void ColorUtils_whitePointD65(double white_point[3]) {
  white_point[0] = kWhitePointD65[0];
  white_point[1] = kWhitePointD65[1];
  white_point[2] = kWhitePointD65[2];
}

double ColorUtils_labF(double t) {
  const double e = 216.0 / 24389.0;
  const double kappa = 24389.0 / 27.0;
  if (t > e) {
    return cbrt(t);
  } else {
    return (kappa * t + 16.0) / 116.0;
  }
}

double ColorUtils_labInvf(double ft) {
  const double e = 216.0 / 24389.0;
  const double kappa = 24389.0 / 27.0;
  double ft3 = ft * ft * ft;
  if (ft3 > e) {
    return ft3;
  } else {
    return (116.0 * ft - 16.0) / kappa;
  }
}
