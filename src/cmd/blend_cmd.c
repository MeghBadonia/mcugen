#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "blend_cmd.h"
#include "generate.h"
#include "../util/log.h"
#include "../core/color.h"
#include "../mcugen.h"
#include "mcu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Blend two ARGB seeds via circular HCT hue average, keep higher chroma */
static int blend_seeds(int argb1, int argb2) {
    Hct h1 = Hct_fromInt(argb1);
    Hct h2 = Hct_fromInt(argb2);

    /* circular mean of hues */
    double a1 = h1.hue * M_PI / 180.0;
    double a2 = h2.hue * M_PI / 180.0;
    double sin_m = (sin(a1) + sin(a2)) / 2.0;
    double cos_m = (cos(a1) + cos(a2)) / 2.0;
    double hue_blend = atan2(sin_m, cos_m) * 180.0 / M_PI;
    if (hue_blend < 0) hue_blend += 360.0;

    /* take max chroma, average tone */
    double chroma = h1.chroma > h2.chroma ? h1.chroma : h2.chroma;
    double tone   = (h1.tone + h2.tone) / 2.0;

    Hct blended = Hct_from(hue_blend, chroma, tone);
    return blended.argb;
}

void cmd_blend(const char *hex1, const char *hex2, int dark, SchemeVariant sv,
               const Config *cfg, int show_only, int as_json, int as_swatch) {
    unsigned long v1 = strtoul(hex1[0]=='#' ? hex1+1 : hex1, NULL, 16);
    unsigned long v2 = strtoul(hex2[0]=='#' ? hex2+1 : hex2, NULL, 16);
    int argb1 = (int)(0xFF000000u | (unsigned)v1);
    int argb2 = (int)(0xFF000000u | (unsigned)v2);

    int blended = blend_seeds(argb1, argb2);

    char h[8]; StringUtils_hexFromArgb(blended, h);

    int r = (blended >> 16) & 0xFF;
    int g = (blended >>  8) & 0xFF;
    int b = (blended      ) & 0xFF;
    printf("Blend: %s + %s → \033[48;2;%d;%d;%dm  \033[0m #%s\n",
           hex1, hex2, r, g, b, h);

    generate(blended, dark, sv, cfg, show_only, as_json, as_swatch, NULL);
}
