/*
 * math_utils.h
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
#ifndef MCU_UTILS_MATH_UTILS_H_
#define MCU_UTILS_MATH_UTILS_H_

/**
 * Utility methods for mathematical operations.
 */

/**
 * Linear interpolation. Returns start if amount=0, stop if amount=1.
 */
double MathUtils_lerp(double start, double stop, double amount);

/**
 * Sanitizes a degree measure as an integer.
 * Returns a degree between 0 (inclusive) and 360 (exclusive).
 */
int MathUtils_sanitizeDegreesInt(int degrees);

/**
 * Sanitizes a degree measure as a floating-point number.
 * Returns a degree between 0.0 (inclusive) and 360.0 (exclusive).
 */
double MathUtils_sanitizeDegreesDouble(double degrees);

/**
 * Clamps a double between min and max.
 */
double MathUtils_clampDouble(double min, double max, double input);

/**
 * Sign of direction change needed to travel from one angle to another.
 * Returns -1.0 or 1.0.
 */
double MathUtils_rotationDirection(double from, double to);

/**
 * Distance of two points on a circle, in degrees.
 */
double MathUtils_differenceDegrees(double a, double b);

/**
 * Multiplies a 1x3 row vector with a 3x3 matrix.
 * result must be a pre-allocated array of 3 doubles.
 */
void MathUtils_matrixMultiply(const double row[3],
                               const double matrix[3][3],
                               double result[3]);

#endif /* MCU_UTILS_MATH_UTILS_H_ */
