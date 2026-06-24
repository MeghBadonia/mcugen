/*
 * math_utils.c
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
#include "math_utils.h"
#include <math.h>

double MathUtils_lerp(double start, double stop, double amount) {
  return (1.0 - amount) * start + amount * stop;
}

int MathUtils_sanitizeDegreesInt(int degrees) {
  degrees = degrees % 360;
  if (degrees < 0) {
    degrees += 360;
  }
  return degrees;
}

double MathUtils_sanitizeDegreesDouble(double degrees) {
  degrees = fmod(degrees, 360.0);
  if (degrees < 0.0) {
    degrees += 360.0;
  }
  return degrees;
}

double MathUtils_clampDouble(double min, double max, double input) {
  if (input < min) return min;
  if (input > max) return max;
  return input;
}

double MathUtils_rotationDirection(double from, double to) {
  double increasing_difference = MathUtils_sanitizeDegreesDouble(to - from);
  return (increasing_difference <= 180.0) ? 1.0 : -1.0;
}

double MathUtils_differenceDegrees(double a, double b) {
  return 180.0 - fabs(fabs(a - b) - 180.0);
}

void MathUtils_matrixMultiply(const double row[3],
                               const double matrix[3][3],
                               double result[3]) {
  result[0] = row[0] * matrix[0][0] + row[1] * matrix[0][1] + row[2] * matrix[0][2];
  result[1] = row[0] * matrix[1][0] + row[1] * matrix[1][1] + row[2] * matrix[1][2];
  result[2] = row[0] * matrix[2][0] + row[1] * matrix[2][1] + row[2] * matrix[2][2];
}
