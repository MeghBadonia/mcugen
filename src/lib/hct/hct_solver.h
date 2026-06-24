/*
 * hct_solver.h
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
#ifndef MCU_HCT_HCT_SOLVER_H_
#define MCU_HCT_HCT_SOLVER_H_

#include "cam16.h"

/**
 * Finds an sRGB color with the given hue, chroma, and L*, if possible.
 *
 * @param hue_degrees  Desired hue, in degrees.
 * @param chroma       Desired chroma.
 * @param lstar        Desired L*.
 * @return ARGB integer of the color. The color has sufficiently close hue,
 *         chroma, and L* to the desired values if possible; otherwise the hue
 *         and L* will be close and chroma will be maximized.
 */
int HctSolver_solveToInt(double hue_degrees, double chroma, double lstar);

/**
 * Same as HctSolver_solveToInt, but returns a Cam16.
 */
Cam16 HctSolver_solveToCam(double hue_degrees, double chroma, double lstar);

#endif /* MCU_HCT_HCT_SOLVER_H_ */
