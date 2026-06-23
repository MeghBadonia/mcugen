/*
 * dislike.h
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
#ifndef MCU_DISLIKE_DISLIKE_H_
#define MCU_DISLIKE_DISLIKE_H_

#include "../hct/hct.h"

/**
 * Check and/or fix universally disliked colors.
 *
 * Research shows universal dislike for dark yellow-greens, correlated with
 * biological waste and rotting food. (Palmer & Schloss 2010.)
 */

/**
 * Returns 1 if the color is universally disliked (dark yellow-green),
 * 0 otherwise.
 */
int DislikeAnalyzer_isDisliked(const Hct *hct);

/**
 * If the color is disliked, returns a lightened version (tone 70).
 * Otherwise returns the color unchanged.
 */
Hct DislikeAnalyzer_fixIfDisliked(const Hct *hct);

#endif /* MCU_DISLIKE_DISLIKE_H_ */
