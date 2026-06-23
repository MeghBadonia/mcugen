/*
 * score.c
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
#include "score.h"
#include "../hct/hct.h"
#include "../utils/math_utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ---- Constants ---- */
#define TARGET_CHROMA             48.0
#define WEIGHT_PROPORTION          0.7
#define WEIGHT_CHROMA_ABOVE        0.3
#define WEIGHT_CHROMA_BELOW        0.1
#define CUTOFF_CHROMA              5.0
#define CUTOFF_EXCITED_PROPORTION  0.01

/* ---- Internal types ---- */
typedef struct {
  Hct    hct;
  double score;
} ScoredHct;

static int scored_hct_compare_desc(const void *a, const void *b) {
  double sa = ((const ScoredHct *)a)->score;
  double sb = ((const ScoredHct *)b)->score;
  /* descending sort: higher score first */
  if (sb > sa) return  1;
  if (sb < sa) return -1;
  return 0;
}

/* ---- Public API ---- */

int Score_score(const int *colors_argb,
                const int *populations,
                int n_colors,
                int desired,
                int fallback_argb,
                int filter,
                int *out_colors) {

  /* 1. Build HCT for each color, accumulate per-hue and total population. */
  Hct  *hcts          = (Hct  *)malloc(n_colors * sizeof(Hct));
  int   hue_pop[360];
  double population_sum = 0.0;
  memset(hue_pop, 0, sizeof(hue_pop));

  for (int i = 0; i < n_colors; i++) {
    hcts[i] = Hct_fromInt(colors_argb[i]);
    int hue_index = (int)floor(hcts[i].hue);
    if (hue_index < 0)   hue_index = 0;
    if (hue_index > 359) hue_index = 359;
    hue_pop[hue_index] += populations[i];
    population_sum     += populations[i];
  }

  /* 2. Compute excited proportions for a ±15° window around each hue. */
  double hue_excited[360];
  memset(hue_excited, 0, sizeof(hue_excited));
  for (int hue = 0; hue < 360; hue++) {
    double proportion = hue_pop[hue] / population_sum;
    for (int i = hue - 14; i < hue + 16; i++) {
      int neighbor = MathUtils_sanitizeDegreesInt(i);
      hue_excited[neighbor] += proportion;
    }
  }

  /* 3. Score each HCT color. */
  ScoredHct *scored = (ScoredHct *)malloc(n_colors * sizeof(ScoredHct));
  int n_scored = 0;
  for (int i = 0; i < n_colors; i++) {
    int hue_idx = MathUtils_sanitizeDegreesInt((int)round(hcts[i].hue));
    double proportion = hue_excited[hue_idx];
    if (filter && (hcts[i].chroma < CUTOFF_CHROMA ||
                   proportion <= CUTOFF_EXCITED_PROPORTION)) {
      continue;
    }
    double proportion_score = proportion * 100.0 * WEIGHT_PROPORTION;
    double chroma_weight    = (hcts[i].chroma < TARGET_CHROMA)
                                  ? WEIGHT_CHROMA_BELOW
                                  : WEIGHT_CHROMA_ABOVE;
    double chroma_score     = (hcts[i].chroma - TARGET_CHROMA) * chroma_weight;
    scored[n_scored].hct   = hcts[i];
    scored[n_scored].score = proportion_score + chroma_score;
    n_scored++;
  }

  /* 4. Sort descending by score. */
  qsort(scored, n_scored, sizeof(ScoredHct), scored_hct_compare_desc);

  /* 5. Pick colors with maximum hue diversity (90° → 15° minimum gap). */
  Hct *chosen   = (Hct *)malloc((desired + 1) * sizeof(Hct));
  int  n_chosen = 0;

  for (int diff_deg = 90; diff_deg >= 15; diff_deg--) {
    n_chosen = 0;
    for (int i = 0; i < n_scored; i++) {
      Hct *candidate = &scored[i].hct;
      int  duplicate = 0;
      for (int j = 0; j < n_chosen; j++) {
        if (MathUtils_differenceDegrees(candidate->hue, chosen[j].hue)
            < diff_deg) {
          duplicate = 1;
          break;
        }
      }
      if (!duplicate) {
        chosen[n_chosen++] = *candidate;
      }
      if (n_chosen >= desired) break;
    }
    if (n_chosen >= desired) break;
  }

  /* 6. Fill output. */
  int n_out = 0;
  if (n_chosen == 0) {
    out_colors[n_out++] = fallback_argb;
  } else {
    for (int i = 0; i < n_chosen; i++) {
      out_colors[n_out++] = chosen[i].argb;
    }
  }

  free(hcts);
  free(scored);
  free(chosen);
  return n_out;
}
