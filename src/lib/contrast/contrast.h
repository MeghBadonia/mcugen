/*
 * contrast.h
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
#ifndef MCU_CONTRAST_CONTRAST_H_
#define MCU_CONTRAST_CONTRAST_H_

/**
 * Color science utilities for contrast.
 *
 * Contrast ratio is defined by WCAG as (lighter_Y + 5) / (darker_Y + 5).
 * HCT tone (= L* from L*a*b*) is used as the perceptual luminance axis.
 */

#define CONTRAST_RATIO_MIN  1.0
#define CONTRAST_RATIO_MAX 21.0
#define CONTRAST_RATIO_30   3.0
#define CONTRAST_RATIO_45   4.5
#define CONTRAST_RATIO_70   7.0

/**
 * Contrast ratio of two relative luminance values (Y in XYZ).
 */
double Contrast_ratioOfYs(double y1, double y2);

/**
 * Contrast ratio of two tones (L* / HCT tone).
 */
double Contrast_ratioOfTones(double t1, double t2);

/**
 * Returns a tone >= tone that achieves the desired contrast ratio.
 * Returns CONTRAST_INVALID (< 0) if the ratio cannot be achieved.
 * Use Contrast_lighterUnsafe to always get a clamped value.
 *
 * @param tone   Tone to contrast against.
 * @param ratio  Desired contrast ratio.
 * @param out    Output tone (only valid when return value is 0).
 * @return 0 on success, -1 if ratio cannot be achieved.
 */
int Contrast_lighter(double tone, double ratio, double *out);

/**
 * Tone >= tone that ensures ratio; 100.0 if ratio cannot be achieved.
 */
double Contrast_lighterUnsafe(double tone, double ratio);

/**
 * Returns a tone <= tone that achieves the desired contrast ratio.
 * @param out  Output tone (only valid when return value is 0).
 * @return 0 on success, -1 if ratio cannot be achieved.
 */
int Contrast_darker(double tone, double ratio, double *out);

/**
 * Tone <= tone that ensures ratio; 0.0 if ratio cannot be achieved.
 */
double Contrast_darkerUnsafe(double tone, double ratio);

#endif /* MCU_CONTRAST_CONTRAST_H_ */
