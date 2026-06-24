#pragma once
#include <stddef.h>

typedef enum { VAR_DEFAULT=0, VAR_LIGHT, VAR_DARK } ColorVariant;

typedef enum {
    FMT_HEX=0, FMT_HEX_STRIP, FMT_HEX_UPPER, FMT_HEX_STRIP_UPPER,
    FMT_RGB, FMT_RGBA, FMT_RGB_RAW,
    FMT_R, FMT_G, FMT_B, FMT_R_FLOAT, FMT_G_FLOAT, FMT_B_FLOAT,
    FMT_ARGB_INT, FMT_ARGB_HEX, FMT_HSL,
    FMT_HCT_HUE, FMT_HCT_CHROMA, FMT_HCT_TONE,
    FMT_UNKNOWN,
} ColorFormat;

ColorFormat   parse_format(const char *s);
ColorVariant  parse_variant(const char *s);
void          rgb_to_hsl(int r, int g, int b, float *h, float *s, float *l);
void          format_color(int argb, ColorFormat fmt, char *buf, size_t sz);
