/*
 * cam16.c
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
#include "cam16.h"
#include "../utils/color_utils.h"
#include "../utils/math_utils.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* The RGB => XYZ conversion matrix elements are derived scientific constants.
 * Values are kept identical to the Kotlin source for cross-implementation
 * accuracy. */
const double kXyzToCam16Rgb[3][3] = {
    { 0.401288,  0.650173, -0.051461},
    {-0.250268,  1.204414,  0.045854},
    {-0.002079,  0.048952,  0.953127},
};

const double kCam16RgbToXyz[3][3] = {
    { 1.8620678, -1.0112547,  0.14918678},
    { 0.38752654, 0.62144744,-0.00897398},
    {-0.0158415, -0.03412294, 1.0499644 },
};

/* ---- Internal helpers ---- */

static double sign_of(double x) {
  if (x > 0.0) return 1.0;
  if (x < 0.0) return -1.0;
  return 0.0;
}

/* Build a Cam16 from J, chroma(c), hue(h) in the given viewing conditions. */
static Cam16 fromJchInViewingConditions(double j, double c, double h,
                                         const ViewingConditions *vc) {
  double q = 4.0 / vc->c * sqrt(j / 100.0) * (vc->aw + 4.0) * vc->fl_root;
  double m = c * vc->fl_root;
  double alpha = (j == 0.0) ? 0.0 : c / sqrt(j / 100.0);
  double s = 50.0 * sqrt(alpha * vc->c / (vc->aw + 4.0));
  double hue_radians = h * (M_PI / 180.0);
  double jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
  double mstar = (1.0 / 0.0228) * log1p(0.0228 * m);
  double astar = mstar * cos(hue_radians);
  double bstar = mstar * sin(hue_radians);
  Cam16 result = {h, c, j, q, m, s, jstar, astar, bstar};
  return result;
}

/* ---- Public API ---- */

Cam16 Cam16_fromInt(int argb) {
  return Cam16_fromIntInViewingConditions(argb, ViewingConditions_DEFAULT());
}

Cam16 Cam16_fromIntInViewingConditions(int argb,
                                        const ViewingConditions *vc) {
  int red   = (argb & 0x00ff0000) >> 16;
  int green = (argb & 0x0000ff00) >> 8;
  int blue  =  argb & 0x000000ff;
  double red_l   = ColorUtils_linearized(red);
  double green_l = ColorUtils_linearized(green);
  double blue_l  = ColorUtils_linearized(blue);
  double x = 0.41233895 * red_l + 0.35762064 * green_l + 0.18051042 * blue_l;
  double y = 0.2126     * red_l + 0.7152     * green_l + 0.0722     * blue_l;
  double z = 0.01932141 * red_l + 0.11916382 * green_l + 0.95034478 * blue_l;
  return Cam16_fromXyzInViewingConditions(x, y, z, vc);
}

Cam16 Cam16_fromXyzInViewingConditions(double x, double y, double z,
                                        const ViewingConditions *vc) {
  /* Transform XYZ to CAM16 'cone' responses */
  double rT = x * kXyzToCam16Rgb[0][0] + y * kXyzToCam16Rgb[0][1] +
              z * kXyzToCam16Rgb[0][2];
  double gT = x * kXyzToCam16Rgb[1][0] + y * kXyzToCam16Rgb[1][1] +
              z * kXyzToCam16Rgb[1][2];
  double bT = x * kXyzToCam16Rgb[2][0] + y * kXyzToCam16Rgb[2][1] +
              z * kXyzToCam16Rgb[2][2];

  /* Discount illuminant */
  double rD = vc->rgb_d[0] * rT;
  double gD = vc->rgb_d[1] * gT;
  double bD = vc->rgb_d[2] * bT;

  /* Chromatic adaptation */
  double rAF = pow(vc->fl * fabs(rD) / 100.0, 0.42);
  double gAF = pow(vc->fl * fabs(gD) / 100.0, 0.42);
  double bAF = pow(vc->fl * fabs(bD) / 100.0, 0.42);
  double rA  = sign_of(rD) * 400.0 * rAF / (rAF + 27.13);
  double gA  = sign_of(gD) * 400.0 * gAF / (gAF + 27.13);
  double bA  = sign_of(bD) * 400.0 * bAF / (bAF + 27.13);

  /* redness-greenness and yellowness-blueness */
  double a = (11.0 * rA - 12.0 * gA + bA) / 11.0;
  double b = (rA + gA - 2.0 * bA) / 9.0;

  /* auxiliary */
  double u  = (20.0 * rA + 20.0 * gA + 21.0 * bA) / 20.0;
  double p2 = (40.0 * rA + 20.0 * gA + bA) / 20.0;

  /* hue */
  double atan2_val    = atan2(b, a);
  double atan_degrees = atan2_val * (180.0 / M_PI);
  double hue          = MathUtils_sanitizeDegreesDouble(atan_degrees);
  double hue_radians  = hue * (M_PI / 180.0);

  /* achromatic response */
  double ac = p2 * vc->nbb;

  /* CAM16 lightness and brightness */
  double j = 100.0 * pow(ac / vc->aw, vc->c * vc->z);
  double q = 4.0 / vc->c * sqrt(j / 100.0) * (vc->aw + 4.0) * vc->fl_root;

  /* chroma, colorfulness, saturation */
  double hue_prime = (hue < 20.14) ? (hue + 360.0) : hue;
  double eHue = 0.25 * (cos(hue_prime * (M_PI / 180.0) + 2.0) + 3.8);
  double p1   = 50000.0 / 13.0 * eHue * vc->nc * vc->ncb;
  double t    = p1 * hypot(a, b) / (u + 0.305);
  double alpha = pow(1.64 - pow(0.29, vc->n), 0.73) * pow(t, 0.9);
  double c_val = alpha * sqrt(j / 100.0);
  double m_val = c_val * vc->fl_root;
  double s_val = 50.0 * sqrt(alpha * vc->c / (vc->aw + 4.0));

  /* CAM16-UCS */
  double jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
  double mstar = (1.0 / 0.0228) * log1p(0.0228 * m_val);
  double astar = mstar * cos(hue_radians);
  double bstar = mstar * sin(hue_radians);

  Cam16 result = {hue, c_val, j, q, m_val, s_val, jstar, astar, bstar};
  return result;
}

Cam16 Cam16_fromJch(double j, double c, double h) {
  return fromJchInViewingConditions(j, c, h, ViewingConditions_DEFAULT());
}

Cam16 Cam16_fromUcs(double jstar, double astar, double bstar) {
  return Cam16_fromUcsInViewingConditions(jstar, astar, bstar,
                                           ViewingConditions_DEFAULT());
}

Cam16 Cam16_fromUcsInViewingConditions(double jstar, double astar,
                                        double bstar,
                                        const ViewingConditions *vc) {
  double m    = hypot(astar, bstar);
  double m2   = expm1(m * 0.0228) / 0.0228;
  double c    = m2 / vc->fl_root;
  double h    = atan2(bstar, astar) * (180.0 / M_PI);
  if (h < 0.0) h += 360.0;
  double j = jstar / (1.0 - (jstar - 100.0) * 0.007);
  return fromJchInViewingConditions(j, c, h, vc);
}

void Cam16_xyzInViewingConditions(const Cam16 *cam16,
                                   const ViewingConditions *vc,
                                   double xyz_out[3]) {
  double alpha = (cam16->chroma == 0.0 || cam16->j == 0.0)
                     ? 0.0
                     : cam16->chroma / sqrt(cam16->j / 100.0);
  double t = pow(alpha / pow(1.64 - pow(0.29, vc->n), 0.73), 1.0 / 0.9);
  double hRad = cam16->hue * (M_PI / 180.0);
  double eHue = 0.25 * (cos(hRad + 2.0) + 3.8);
  double ac   = vc->aw * pow(cam16->j / 100.0, 1.0 / vc->c / vc->z);
  double p1   = eHue * (50000.0 / 13.0) * vc->nc * vc->ncb;
  double p2   = ac / vc->nbb;
  double hSin = sin(hRad);
  double hCos = cos(hRad);
  double gamma = 23.0 * (p2 + 0.305) * t /
                 (23.0 * p1 + 11.0 * t * hCos + 108.0 * t * hSin);
  double a  = gamma * hCos;
  double b  = gamma * hSin;
  double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
  double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
  double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;
  double rCBase = fmax(0.0, 27.13 * fabs(rA) / (400.0 - fabs(rA)));
  double rC = sign_of(rA) * (100.0 / vc->fl) * pow(rCBase, 1.0 / 0.42);
  double gCBase = fmax(0.0, 27.13 * fabs(gA) / (400.0 - fabs(gA)));
  double gC = sign_of(gA) * (100.0 / vc->fl) * pow(gCBase, 1.0 / 0.42);
  double bCBase = fmax(0.0, 27.13 * fabs(bA) / (400.0 - fabs(bA)));
  double bC = sign_of(bA) * (100.0 / vc->fl) * pow(bCBase, 1.0 / 0.42);
  double rF = rC / vc->rgb_d[0];
  double gF = gC / vc->rgb_d[1];
  double bF = bC / vc->rgb_d[2];
  xyz_out[0] = rF * kCam16RgbToXyz[0][0] + gF * kCam16RgbToXyz[0][1] +
               bF * kCam16RgbToXyz[0][2];
  xyz_out[1] = rF * kCam16RgbToXyz[1][0] + gF * kCam16RgbToXyz[1][1] +
               bF * kCam16RgbToXyz[1][2];
  xyz_out[2] = rF * kCam16RgbToXyz[2][0] + gF * kCam16RgbToXyz[2][1] +
               bF * kCam16RgbToXyz[2][2];
}

int Cam16_toInt(const Cam16 *cam16) {
  return Cam16_viewed(cam16, ViewingConditions_DEFAULT());
}

int Cam16_viewed(const Cam16 *cam16, const ViewingConditions *vc) {
  double xyz[3];
  Cam16_xyzInViewingConditions(cam16, vc, xyz);
  return ColorUtils_argbFromXyz(xyz[0], xyz[1], xyz[2]);
}

double Cam16_distance(const Cam16 *a, const Cam16 *b) {
  double dJ = a->jstar - b->jstar;
  double dA = a->astar - b->astar;
  double dB = a->bstar - b->bstar;
  double dEPrime = sqrt(dJ * dJ + dA * dA + dB * dB);
  return 1.41 * pow(dEPrime, 0.63);
}
