/*
 * blend.h
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
#ifndef MCU_BLEND_BLEND_H_
#define MCU_BLEND_BLEND_H_

/**
 * Functions for blending in HCT and CAM16.
 */

/**
 * Blend the design color's HCT hue towards the key color's HCT hue,
 * keeping the design color recognizable but shifted towards the key.
 *
 * @param design_color ARGB of the color to be shifted.
 * @param source_color ARGB of the main theme / key color.
 * @return ARGB with hue shifted towards source_color.
 */
int Blend_harmonize(int design_color, int source_color);

/**
 * Blend hue from one color into another; chroma and tone are preserved.
 *
 * @param from   ARGB source color.
 * @param to     ARGB target color.
 * @param amount 0.0–1.0 blend factor.
 * @return from, with hue blended towards to.
 */
int Blend_hctHue(int from, int to, double amount);

/**
 * Blend two colors in CAM16-UCS space.
 *
 * @param from   ARGB source color.
 * @param to     ARGB target color.
 * @param amount 0.0–1.0 blend factor.
 * @return Blend of from towards to (hue, chroma, and tone all change).
 */
int Blend_cam16Ucs(int from, int to, double amount);

#endif /* MCU_BLEND_BLEND_H_ */
