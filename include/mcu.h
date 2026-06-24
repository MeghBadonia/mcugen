/*
 * demo.c
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

/*
 * material_color_utilities — C port
 * Umbrella header: include this to get everything.
 *
 * Build order / dependency tree:
 *   utils/math_utils
 *   utils/color_utils   <- math_utils
 *   utils/string_utils  <- color_utils
 *   hct/viewing_conditions <- color_utils, math_utils, cam16 (matrix only)
 *   hct/cam16           <- viewing_conditions, color_utils, math_utils
 *   hct/hct_solver      <- cam16, viewing_conditions, color_utils, math_utils
 *   hct/hct             <- cam16, hct_solver, color_utils, viewing_conditions
 *   blend/blend         <- hct, cam16, color_utils, math_utils
 *   contrast/contrast   <- color_utils
 *   dislike/dislike     <- hct
 *   palettes/tonal_palette  <- hct, color_utils
 *   palettes/core_palettes  <- tonal_palette
 *   score/score         <- hct, math_utils
 *   temperature/temperature_cache <- hct, color_utils, math_utils
 */
#ifndef MCU_H_
#define MCU_H_

#include "utils/math_utils.h"
#include "utils/color_utils.h"
#include "utils/string_utils.h"
#include "hct/viewing_conditions.h"
#include "hct/cam16.h"
#include "hct/hct_solver.h"
#include "hct/hct.h"
#include "blend/blend.h"
#include "contrast/contrast.h"
#include "dislike/dislike.h"
#include "palettes/tonal_palette.h"
#include "palettes/core_palettes.h"
#include "score/score.h"
#include "temperature/temperature_cache.h"

#endif /* MCU_H_ */
