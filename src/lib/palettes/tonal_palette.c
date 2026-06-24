/*
 * tonal_palette.c
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
#include "tonal_palette.h"
#include "../utils/color_utils.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ---- Internal helpers ---- */

static int average_argb(int argb1, int argb2) {
  int r1 = (argb1 >> 16) & 0xff;
  int g1 = (argb1 >>  8) & 0xff;
  int b1 =  argb1        & 0xff;
  int r2 = (argb2 >> 16) & 0xff;
  int g2 = (argb2 >>  8) & 0xff;
  int b2 =  argb2        & 0xff;
  int r  = (int)round((r1 + r2) / 2.0f);
  int g  = (int)round((g1 + g2) / 2.0f);
  int b  = (int)round((b1 + b2) / 2.0f);
  return (255 << 24) | ((r & 255) << 16) | ((g & 255) << 8) | (b & 255);
}

/*
 * KeyColor: find the first tone starting from T50 that provides the requested
 * chroma. Uses a binary search around the chroma peak.
 */
#define MAX_CHROMA_VALUE 200.0

static Hct key_color_create(double hue, double requested_chroma) {
  /* Cache: max chroma at each integer tone 0-100.
   * 0 means "not computed"; chroma is always >= 0 so we use -1 as sentinel. */
  double chroma_cache[101];
  for (int i = 0; i <= 100; i++) chroma_cache[i] = -1.0;

  /* Helper: max chroma at a tone (lazy cache) */
#define MAX_CHROMA(t) \
  (chroma_cache[t] < 0.0 \
       ? (chroma_cache[t] = Hct_from(hue, MAX_CHROMA_VALUE, (double)(t)).chroma) \
       : chroma_cache[t])

  const int pivot_tone   = 50;
  const int tone_step    = 1;
  const double epsilon   = 0.01;

  int lower_tone = 0;
  int upper_tone = 100;

  while (lower_tone < upper_tone) {
    int mid_tone = (lower_tone + upper_tone) / 2;
    int is_ascending = MAX_CHROMA(mid_tone) < MAX_CHROMA(mid_tone + tone_step);
    int sufficient   = MAX_CHROMA(mid_tone) >= requested_chroma - epsilon;
    if (sufficient) {
      int dist_lower = abs(lower_tone - pivot_tone);
      int dist_upper = abs(upper_tone - pivot_tone);
      if (dist_lower < dist_upper) {
        upper_tone = mid_tone;
      } else {
        if (lower_tone == mid_tone) {
          return Hct_from(hue, requested_chroma, (double)lower_tone);
        }
        lower_tone = mid_tone;
      }
    } else {
      if (is_ascending) {
        lower_tone = mid_tone + tone_step;
      } else {
        upper_tone = mid_tone;
      }
    }
  }
#undef MAX_CHROMA
  return Hct_from(hue, requested_chroma, (double)lower_tone);
}

/* Initialize the cache slots to "uncached" (-1). */
static void init_cache(TonalPalette *p) {
  for (int i = 0; i <= 100; i++) p->cache[i] = -1;
}

/* ---- Public API ---- */

TonalPalette TonalPalette_fromInt(int argb) {
  Hct hct = Hct_fromInt(argb);
  return TonalPalette_fromHct(&hct);
}

TonalPalette TonalPalette_fromHct(const Hct *hct) {
  TonalPalette p;
  p.hue       = hct->hue;
  p.chroma    = hct->chroma;
  p.key_color = *hct;
  init_cache(&p);
  return p;
}

TonalPalette TonalPalette_fromHueAndChroma(double hue, double chroma) {
  TonalPalette p;
  p.hue       = hue;
  p.chroma    = chroma;
  p.key_color = key_color_create(hue, chroma);
  init_cache(&p);
  return p;
}

int TonalPalette_tone(TonalPalette *palette, int tone) {
  if (tone < 0) tone = 0;
  if (tone > 100) tone = 100;
  if (palette->cache[tone] != -1) return palette->cache[tone];

  int color;
  if (tone == 99 && Hct_isYellow(palette->hue)) {
    /* Yellow at T99 looks off; average T98 and T100. */
    color = average_argb(TonalPalette_tone(palette, 98),
                          TonalPalette_tone(palette, 100));
  } else {
    color = Hct_from(palette->hue, palette->chroma, (double)tone).argb;
  }
  palette->cache[tone] = color;
  return color;
}

Hct TonalPalette_getHct(TonalPalette *palette, double tone) {
  if (tone == 99.0 && Hct_isYellow(palette->hue)) {
    return Hct_fromInt(TonalPalette_tone(palette, 99));
  }
  return Hct_from(palette->hue, palette->chroma, tone);
}
