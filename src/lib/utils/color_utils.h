/*
 * color_utils.h
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
#ifndef MCU_UTILS_COLOR_UTILS_H_
#define MCU_UTILS_COLOR_UTILS_H_

/**
 * Color science utilities.
 *
 * Utility methods for color science constants and color space conversions
 * that aren't HCT or CAM16.
 */

/** Converts a color from RGB components to ARGB format. */
int ColorUtils_argbFromRgb(int red, int green, int blue);

/**
 * Converts a color from linear RGB components (array of 3 doubles) to ARGB
 * format.
 */
int ColorUtils_argbFromLinrgb(const double linrgb[3]);

/** Returns the alpha component of a color in ARGB format. */
int ColorUtils_alphaFromArgb(int argb);

/** Returns the red component of a color in ARGB format. */
int ColorUtils_redFromArgb(int argb);

/** Returns the green component of a color in ARGB format. */
int ColorUtils_greenFromArgb(int argb);

/** Returns the blue component of a color in ARGB format. */
int ColorUtils_blueFromArgb(int argb);

/** Returns 1 if the color is opaque (alpha >= 255), 0 otherwise. */
int ColorUtils_isOpaque(int argb);

/** Converts XYZ to ARGB. */
int ColorUtils_argbFromXyz(double x, double y, double z);

/**
 * Converts ARGB to XYZ. Fills xyz[3] with the result.
 */
void ColorUtils_xyzFromArgb(int argb, double xyz[3]);

/** Converts Lab to ARGB. */
int ColorUtils_argbFromLab(double l, double a, double b);

/**
 * Converts ARGB to Lab. Fills lab[3] with (L, a, b).
 */
void ColorUtils_labFromArgb(int argb, double lab[3]);

/** Converts an L* value to an ARGB representation of a grayscale color. */
int ColorUtils_argbFromLstar(double lstar);

/**
 * Computes the L* value of a color in ARGB representation.
 */
double ColorUtils_lstarFromArgb(int argb);

/**
 * Converts an L* value to a Y value (XYZ).
 */
double ColorUtils_yFromLstar(double lstar);

/**
 * Converts a Y value (XYZ) to an L* value.
 */
double ColorUtils_lstarFromY(double y);

/**
 * Linearizes an RGB component (0-255) to linear light (0.0-100.0).
 */
double ColorUtils_linearized(int rgb_component);

/**
 * Delinearizes a linear RGB component (0.0-100.0) to 0-255.
 */
int ColorUtils_delinearized(double rgb_component);

/**
 * Returns the standard D65 white point. Fills white_point[3].
 */
void ColorUtils_whitePointD65(double white_point[3]);

/** Internal Lab helper: f function. */
double ColorUtils_labF(double t);

/** Internal Lab helper: inverse f function. */
double ColorUtils_labInvf(double ft);

#endif /* MCU_UTILS_COLOR_UTILS_H_ */
