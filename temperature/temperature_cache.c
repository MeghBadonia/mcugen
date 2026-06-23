/*
 * temperature_cache.c
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
#include "temperature_cache.h"
#include "../utils/color_utils.h"
#include "../utils/math_utils.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <stdlib.h>
#include <string.h>

/* ---- Internal struct ---- */

/*
 * hcts_by_hue[i] = HCT at integer hue i (0-360), same chroma/tone as input.
 * hcts_by_temp   = same 361 colors sorted coldest-first (heap allocated).
 * temps_by_hue[i]= raw temperature at hue i.
 */
struct TemperatureCache {
  Hct    input;
  Hct    hcts_by_hue[361];  /* index = integer hue 0..360 */
  double temps_by_hue[361]; /* parallel temperatures */
  /* sorted by temp (coldest first) */
  Hct   *hcts_by_temp;      /* heap, length 362 (361 + input) */
  int    n_by_temp;
  /* complement, computed lazily (-1 argb means not yet computed) */
  int    complement_computed;
  Hct    complement;
};

/* ---- rawTemperature (static/public helper) ---- */

double TemperatureCache_rawTemperature(const Hct *hct) {
  double lab[3];
  ColorUtils_labFromArgb(hct->argb, lab);
  double hue    = MathUtils_sanitizeDegreesDouble(
      atan2(lab[2], lab[1]) * (180.0 / M_PI));
  double chroma = hypot(lab[1], lab[2]);
  return -0.5 + 0.02 * pow(chroma, 1.07) *
         cos(MathUtils_sanitizeDegreesDouble(hue - 50.0) * (M_PI / 180.0));
}

/* ---- Sort helpers ---- */

/* We sort indices into hcts_by_temp by temperature. */
typedef struct { double temp; int orig_idx; } TempEntry;

static int temp_entry_cmp(const void *a, const void *b) {
  double ta = ((const TempEntry *)a)->temp;
  double tb = ((const TempEntry *)b)->temp;
  if (ta < tb) return -1;
  if (ta > tb) return  1;
  return 0;
}

/* ---- Internal precomputation ---- */

static void precompute(TemperatureCache *c) {
  /* Build hcts_by_hue and temps_by_hue */
  for (int i = 0; i <= 360; i++) {
    c->hcts_by_hue[i]  = Hct_from((double)i, c->input.chroma, c->input.tone);
    c->temps_by_hue[i] = TemperatureCache_rawTemperature(&c->hcts_by_hue[i]);
  }

  /* Build hcts_by_temp: copy 361 by-hue entries + the input color */
  int total = 362;
  c->n_by_temp    = total;
  c->hcts_by_temp = (Hct *)malloc(total * sizeof(Hct));

  TempEntry *entries = (TempEntry *)malloc(total * sizeof(TempEntry));
  for (int i = 0; i < 361; i++) {
    entries[i].temp     = c->temps_by_hue[i];
    entries[i].orig_idx = i; /* 0-360 map to hcts_by_hue */
  }
  /* index 361 = input color itself */
  entries[361].temp     = TemperatureCache_rawTemperature(&c->input);
  entries[361].orig_idx = 361;

  qsort(entries, total, sizeof(TempEntry), temp_entry_cmp);

  for (int i = 0; i < total; i++) {
    int idx = entries[i].orig_idx;
    c->hcts_by_temp[i] = (idx < 361) ? c->hcts_by_hue[idx] : c->input;
  }
  free(entries);
}

/* ---- Lookup temp for an arbitrary Hct (search by argb in hcts_by_hue +
   input) ---- */

static double temp_for_hct(const TemperatureCache *c, const Hct *hct) {
  /* Match by argb value */
  if (hct->argb == c->input.argb) {
    return TemperatureCache_rawTemperature(&c->input);
  }
  for (int i = 0; i <= 360; i++) {
    if (c->hcts_by_hue[i].argb == hct->argb) {
      return c->temps_by_hue[i];
    }
  }
  /* Fallback: compute directly */
  return TemperatureCache_rawTemperature(hct);
}

/* ---- isBetween (clockwise) ---- */

static int is_between(double angle, double a, double b) {
  if (a < b) return a <= angle && angle <= b;
  return a <= angle || angle <= b;
}

/* ---- Public API ---- */

TemperatureCache *TemperatureCache_create(Hct input) {
  TemperatureCache *c = (TemperatureCache *)malloc(sizeof(TemperatureCache));
  memset(c, 0, sizeof(TemperatureCache));
  c->input              = input;
  c->complement_computed = 0;
  precompute(c);
  return c;
}

void TemperatureCache_free(TemperatureCache *cache) {
  if (cache) {
    free(cache->hcts_by_temp);
    free(cache);
  }
}

double TemperatureCache_getRelativeTemperature(TemperatureCache *cache,
                                                const Hct *hct) {
  Hct *coldest = &cache->hcts_by_temp[0];
  Hct *warmest = &cache->hcts_by_temp[cache->n_by_temp - 1];
  double cold_temp = temp_for_hct(cache, coldest);
  double warm_temp = temp_for_hct(cache, warmest);
  double range     = warm_temp - cold_temp;
  if (range == 0.0) return 0.5;
  return (temp_for_hct(cache, hct) - cold_temp) / range;
}

Hct TemperatureCache_complement(TemperatureCache *cache) {
  if (cache->complement_computed) return cache->complement;

  Hct *coldest   = &cache->hcts_by_temp[0];
  Hct *warmest   = &cache->hcts_by_temp[cache->n_by_temp - 1];
  double cold_temp = temp_for_hct(cache, coldest);
  double warm_temp = temp_for_hct(cache, warmest);
  double range     = warm_temp - cold_temp;

  double coldest_hue = coldest->hue;
  double warmest_hue = warmest->hue;

  int start_is_cold_to_warm =
      is_between(cache->input.hue, coldest_hue, warmest_hue);
  double start_hue = start_is_cold_to_warm ? warmest_hue : coldest_hue;
  double end_hue   = start_is_cold_to_warm ? coldest_hue : warmest_hue;

  double complement_relative_temp =
      1.0 - TemperatureCache_getRelativeTemperature(cache, &cache->input);

  double smallest_error = 1000.0;
  Hct    answer         = cache->hcts_by_hue[(int)round(cache->input.hue)];

  for (double hue_addend = 0.0; hue_addend <= 360.0; hue_addend += 1.0) {
    double hue = MathUtils_sanitizeDegreesDouble(start_hue + hue_addend);
    if (!is_between(hue, start_hue, end_hue)) continue;
    Hct *possible = &cache->hcts_by_hue[(int)round(hue)];
    double rel_temp = (temp_for_hct(cache, possible) - cold_temp) / range;
    double error    = fabs(complement_relative_temp - rel_temp);
    if (error < smallest_error) {
      smallest_error = error;
      answer         = *possible;
    }
  }

  cache->complement          = answer;
  cache->complement_computed = 1;
  return cache->complement;
}

int TemperatureCache_getAnalogousColors(TemperatureCache *cache,
                                         Hct out_colors[5]) {
  return TemperatureCache_getAnalogousColorsN(cache, 5, 12, out_colors);
}

int TemperatureCache_getAnalogousColorsN(TemperatureCache *cache,
                                          int count,
                                          int divisions,
                                          Hct *out_colors) {
  int start_hue = (int)round(cache->input.hue);
  Hct start_hct = cache->hcts_by_hue[start_hue];

  /* Compute total absolute temperature delta around the wheel */
  double last_temp            = TemperatureCache_getRelativeTemperature(
      cache, &start_hct);
  double absolute_total_delta = 0.0;
  for (int i = 0; i < 360; i++) {
    int  hue  = MathUtils_sanitizeDegreesInt(start_hue + i);
    Hct *h    = &cache->hcts_by_hue[hue];
    double t  = TemperatureCache_getRelativeTemperature(cache, h);
    absolute_total_delta += fabs(t - last_temp);
    last_temp = t;
  }

  /* Collect `divisions` colors equidistant in temperature */
  Hct   *all_colors = (Hct *)malloc((divisions + 2) * sizeof(Hct));
  int    n_all      = 0;
  all_colors[n_all++] = start_hct;

  double temp_step       = (divisions > 0)
                               ? absolute_total_delta / (double)divisions
                               : 0.0;
  double total_temp_delta = 0.0;
  last_temp = TemperatureCache_getRelativeTemperature(cache, &start_hct);

  int hue_addend = 1;
  while (n_all < divisions) {
    int  hue = MathUtils_sanitizeDegreesInt(start_hue + hue_addend);
    Hct *h   = &cache->hcts_by_hue[hue];
    double t = TemperatureCache_getRelativeTemperature(cache, h);
    total_temp_delta += fabs(t - last_temp);

    double desired_delta = n_all * temp_step;
    int    index_satisfied = total_temp_delta >= desired_delta;
    int    index_addend    = 1;

    while (index_satisfied && n_all < divisions) {
      if (n_all < divisions) all_colors[n_all++] = *h;
      double next_desired = (n_all + index_addend) * temp_step;
      index_satisfied     = total_temp_delta >= next_desired;
      index_addend++;
    }

    last_temp = t;
    hue_addend++;

    if (hue_addend > 360) {
      while (n_all < divisions) {
        all_colors[n_all++] = *h;
      }
      break;
    }
  }

  /* Build the final `count`-color answer: input at center, ccw + cw wings */
  int ccw_count = (int)floor((count - 1.0) / 2.0);
  int cw_count  = count - ccw_count - 1;
  int n_out     = 0;
  Hct *answers  = (Hct *)malloc((count + 1) * sizeof(Hct));

  /* Center = input */
  answers[n_out++] = cache->input;

  /* Counter-clockwise wing (prepend) */
  /* We'll build a temporary forward list, then reverse-insert */
  Hct *ccw = (Hct *)malloc(ccw_count * sizeof(Hct));
  for (int i = 1; i <= ccw_count; i++) {
    int index = 0 - i;
    while (index < 0)       index += n_all;
    if (index >= n_all) index %= n_all;
    ccw[i - 1] = all_colors[index];
  }
  /* Prepend ccw (in order ccw_count-1 → 0) */
  Hct *final_out = (Hct *)malloc((count + 1) * sizeof(Hct));
  int  fo_n      = 0;
  for (int i = ccw_count - 1; i >= 0; i--) final_out[fo_n++] = ccw[i];
  /* Add input */
  final_out[fo_n++] = cache->input;
  /* Clockwise wing */
  for (int i = 1; i <= cw_count; i++) {
    int index = i;
    while (index < 0)       index += n_all;
    if (index >= n_all) index %= n_all;
    final_out[fo_n++] = all_colors[index];
  }

  int written = (fo_n < count) ? fo_n : count;
  memcpy(out_colors, final_out, written * sizeof(Hct));

  free(ccw);
  free(answers);
  free(final_out);
  free(all_colors);
  return written;
}
