/*
 * blend.c
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
#include "blend.h"
#include "../hct/cam16.h"
#include "../hct/hct.h"
#include "../utils/color_utils.h"
#include "../utils/math_utils.h"
#include <math.h>

int Blend_harmonize(int design_color, int source_color) {
  Hct from_hct = Hct_fromInt(design_color);
  Hct to_hct   = Hct_fromInt(source_color);
  double diff     = MathUtils_differenceDegrees(from_hct.hue, to_hct.hue);
  double rotation = fmin(diff * 0.5, 15.0);
  double output_hue = MathUtils_sanitizeDegreesDouble(
      from_hct.hue +
      rotation * MathUtils_rotationDirection(from_hct.hue, to_hct.hue));
  Hct result = Hct_from(output_hue, from_hct.chroma, from_hct.tone);
  return result.argb;
}

int Blend_cam16Ucs(int from, int to, double amount) {
  Cam16 from_cam = Cam16_fromInt(from);
  Cam16 to_cam   = Cam16_fromInt(to);
  double jstar   = MathUtils_lerp(from_cam.jstar, to_cam.jstar, amount);
  double astar   = MathUtils_lerp(from_cam.astar, to_cam.astar, amount);
  double bstar   = MathUtils_lerp(from_cam.bstar, to_cam.bstar, amount);
  Cam16 blended  = Cam16_fromUcs(jstar, astar, bstar);
  return Cam16_toInt(&blended);
}

int Blend_hctHue(int from, int to, double amount) {
  int ucs        = Blend_cam16Ucs(from, to, amount);
  Cam16 ucs_cam  = Cam16_fromInt(ucs);
  Cam16 from_cam = Cam16_fromInt(from);
  Hct blended    = Hct_from(ucs_cam.hue, from_cam.chroma,
                              ColorUtils_lstarFromArgb(from));
  return blended.argb;
}
