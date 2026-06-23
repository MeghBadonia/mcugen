/*
 * hct.c
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
#include "hct.h"
#include "cam16.h"
#include "hct_solver.h"
#include "../utils/color_utils.h"

static Hct hct_from_argb(int argb) {
  Hct h;
  h.argb   = argb;
  Cam16 cam = Cam16_fromInt(argb);
  h.hue    = cam.hue;
  h.chroma = cam.chroma;
  h.tone   = ColorUtils_lstarFromArgb(argb);
  return h;
}

Hct Hct_from(double hue, double chroma, double tone) {
  int argb = HctSolver_solveToInt(hue, chroma, tone);
  return hct_from_argb(argb);
}

Hct Hct_fromInt(int argb) {
  return hct_from_argb(argb);
}

int Hct_toInt(const Hct *hct) {
  return hct->argb;
}

Hct Hct_setHue(const Hct *hct, double new_hue) {
  return hct_from_argb(HctSolver_solveToInt(new_hue, hct->chroma, hct->tone));
}

Hct Hct_setChroma(const Hct *hct, double new_chroma) {
  return hct_from_argb(HctSolver_solveToInt(hct->hue, new_chroma, hct->tone));
}

Hct Hct_setTone(const Hct *hct, double new_tone) {
  return hct_from_argb(HctSolver_solveToInt(hct->hue, hct->chroma, new_tone));
}

Hct Hct_inViewingConditions(const Hct *hct, const ViewingConditions *vc) {
  /* 1. Use CAM16 to find XYZ in specified VC. */
  Cam16 cam16 = Cam16_fromInt(Hct_toInt(hct));
  double xyz[3];
  Cam16_xyzInViewingConditions(&cam16, vc, xyz);

  /* 2. Create CAM16 of those XYZ coords in default VC. */
  Cam16 recast = Cam16_fromXyzInViewingConditions(
      xyz[0], xyz[1], xyz[2], ViewingConditions_DEFAULT());

  /* 3. Build HCT from recast CAM16 and the L* from Y in specified VC. */
  return Hct_from(recast.hue, recast.chroma,
                  ColorUtils_lstarFromY(xyz[1]));
}

int Hct_isBlue(double hue)   { return hue >= 250.0 && hue < 270.0; }
int Hct_isYellow(double hue) { return hue >= 105.0 && hue < 125.0; }
int Hct_isCyan(double hue)   { return hue >= 170.0 && hue < 207.0; }
