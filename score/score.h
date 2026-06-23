/*
 * score.h
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
#ifndef MCU_SCORE_SCORE_H_
#define MCU_SCORE_SCORE_H_

#include <stddef.h>

/**
 * Given a large set of colors and their population counts, remove colors
 * unsuitable for a UI theme and rank the rest.
 *
 * @param colors_argb     Array of ARGB color integers.
 * @param populations     Parallel array of population counts.
 * @param n_colors        Number of entries in the above arrays.
 * @param desired         Maximum number of results to return.
 * @param fallback_argb   Color to use if no suitable colors are found.
 *                        Default: 0xff4285f4 (Google Blue).
 * @param filter          1 = filter low-chroma / low-population colors.
 * @param out_colors      Caller-allocated output array (at least `desired`
 *                        ints). The most suitable color is at index 0.
 * @return                Number of colors written to out_colors (>= 1).
 */
int Score_score(const int *colors_argb,
                const int *populations,
                int n_colors,
                int desired,
                int fallback_argb,
                int filter,
                int *out_colors);

#endif /* MCU_SCORE_SCORE_H_ */
