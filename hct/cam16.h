/*
 * cam16.h
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
#ifndef MCU_HCT_CAM16_H_
#define MCU_HCT_CAM16_H_

#include "viewing_conditions.h"

/**
 * CAM16 color appearance model.
 *
 * Colors are defined by their hex code AND viewing conditions.
 * CAM16 instances also carry CAM16-UCS coordinates (jstar, astar, bstar).
 */
typedef struct {
  double hue;    /**< Hue in CAM16 */
  double chroma; /**< Chroma in CAM16 */
  double j;      /**< Lightness in CAM16 */
  double q;      /**< Brightness in CAM16 */
  double m;      /**< Colorfulness in CAM16 */
  double s;      /**< Saturation in CAM16 */
  double jstar;  /**< CAM16-UCS lightness coordinate */
  double astar;  /**< CAM16-UCS a* coordinate */
  double bstar;  /**< CAM16-UCS b* coordinate */
} Cam16;

/**
 * XYZ -> CAM16 'cone'/'rgb' response matrix.
 * Exposed for use by ViewingConditions.
 */
extern const double kXyzToCam16Rgb[3][3];

/**
 * CAM16 'cone'/'rgb' -> XYZ matrix.
 */
extern const double kCam16RgbToXyz[3][3];

/**
 * Create a CAM16 color from an ARGB integer, in default viewing conditions.
 */
Cam16 Cam16_fromInt(int argb);

/**
 * Create a CAM16 color from an ARGB integer in specified viewing conditions.
 */
Cam16 Cam16_fromIntInViewingConditions(int argb,
                                        const ViewingConditions *vc);

/**
 * Create a CAM16 color from XYZ coordinates in specified viewing conditions.
 */
Cam16 Cam16_fromXyzInViewingConditions(double x, double y, double z,
                                        const ViewingConditions *vc);

/**
 * Create a CAM16 color from J, chroma, hue in default viewing conditions.
 */
Cam16 Cam16_fromJch(double j, double c, double h);

/**
 * Create a CAM16 color from CAM16-UCS coordinates in default viewing
 * conditions.
 */
Cam16 Cam16_fromUcs(double jstar, double astar, double bstar);

/**
 * Create a CAM16 color from CAM16-UCS coordinates in specified viewing
 * conditions.
 */
Cam16 Cam16_fromUcsInViewingConditions(double jstar, double astar,
                                        double bstar,
                                        const ViewingConditions *vc);

/**
 * Returns the ARGB integer for this CAM16 color in default viewing conditions.
 */
int Cam16_toInt(const Cam16 *cam16);

/**
 * Returns the ARGB integer for this CAM16 color in specified viewing
 * conditions.
 */
int Cam16_viewed(const Cam16 *cam16, const ViewingConditions *vc);

/**
 * Computes XYZ coordinates of this CAM16 color in specified viewing conditions.
 * If xyz_out is non-NULL, fills it and returns xyz_out; otherwise fills a
 * static buffer (not thread-safe).
 */
void Cam16_xyzInViewingConditions(const Cam16 *cam16,
                                   const ViewingConditions *vc,
                                   double xyz_out[3]);

/**
 * CAM16-UCS perceptual distance between two CAM16 colors.
 */
double Cam16_distance(const Cam16 *a, const Cam16 *b);

#endif /* MCU_HCT_CAM16_H_ */
