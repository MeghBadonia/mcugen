#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "color.h"
#include "../util/log.h"
#include "mcu.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

ColorFormat parse_format(const char *s) {
    if (!s) return FMT_HEX;
    if (!strcmp(s,"hex"))              return FMT_HEX;
    if (!strcmp(s,"hex_strip") || !strcmp(s,"hex_stripped")) return FMT_HEX_STRIP;
    if (!strcmp(s,"HEX") || !strcmp(s,"hex_upper"))          return FMT_HEX_UPPER;
    if (!strcmp(s,"HEX_STRIP") || !strcmp(s,"hex_strip_upper")) return FMT_HEX_STRIP_UPPER;
    if (!strcmp(s,"rgb"))              return FMT_RGB;
    if (!strcmp(s,"rgba"))             return FMT_RGBA;
    if (!strcmp(s,"rgb_raw"))          return FMT_RGB_RAW;
    if (!strcmp(s,"r"))                return FMT_R;
    if (!strcmp(s,"g"))                return FMT_G;
    if (!strcmp(s,"b"))                return FMT_B;
    if (!strcmp(s,"r_float"))          return FMT_R_FLOAT;
    if (!strcmp(s,"g_float"))          return FMT_G_FLOAT;
    if (!strcmp(s,"b_float"))          return FMT_B_FLOAT;
    if (!strcmp(s,"argb_int"))         return FMT_ARGB_INT;
    if (!strcmp(s,"argb_hex"))         return FMT_ARGB_HEX;
    if (!strcmp(s,"hsl"))              return FMT_HSL;
    if (!strcmp(s,"hct_hue"))          return FMT_HCT_HUE;
    if (!strcmp(s,"hct_chroma"))       return FMT_HCT_CHROMA;
    if (!strcmp(s,"hct_tone"))         return FMT_HCT_TONE;
    return FMT_UNKNOWN;
}

ColorVariant parse_variant(const char *s) {
    if (!s || !strcmp(s,"default")) return VAR_DEFAULT;
    if (!strcmp(s,"light"))         return VAR_LIGHT;
    if (!strcmp(s,"dark"))          return VAR_DARK;
    return VAR_DEFAULT;
}

void rgb_to_hsl(int r, int g, int b, float *h, float *s, float *l) {
    float rf=r/255.0f, gf=g/255.0f, bf=b/255.0f;
    float mx=rf>gf?(rf>bf?rf:bf):(gf>bf?gf:bf);
    float mn=rf<gf?(rf<bf?rf:bf):(gf<bf?gf:bf);
    *l = (mx+mn)*0.5f;
    float d = mx-mn;
    if (d<1e-6f) { *h=*s=0.0f; return; }
    *s = d / (1.0f - fabsf(2.0f*(*l) - 1.0f));
    if (mx==rf)      *h = fmodf((gf-bf)/d, 6.0f)*60.0f;
    else if (mx==gf) *h = ((bf-rf)/d + 2.0f)*60.0f;
    else             *h = ((rf-gf)/d + 4.0f)*60.0f;
    if (*h < 0.0f) *h += 360.0f;
}

void format_color(int argb, ColorFormat fmt, char *buf, size_t sz) {
    int r = ColorUtils_redFromArgb(argb),
        g = ColorUtils_greenFromArgb(argb),
        b = ColorUtils_blueFromArgb(argb),
        a = ColorUtils_alphaFromArgb(argb);
    switch (fmt) {
    case FMT_HEX:             snprintf(buf,sz,"#%02x%02x%02x",r,g,b); break;
    case FMT_HEX_STRIP:       snprintf(buf,sz,"%02x%02x%02x",r,g,b); break;
    case FMT_HEX_UPPER:       snprintf(buf,sz,"#%02X%02X%02X",r,g,b); break;
    case FMT_HEX_STRIP_UPPER: snprintf(buf,sz,"%02X%02X%02X",r,g,b); break;
    case FMT_RGB:             snprintf(buf,sz,"rgb(%d, %d, %d)",r,g,b); break;
    case FMT_RGBA:            snprintf(buf,sz,"rgba(%d, %d, %d, %.2f)",r,g,b,a/255.0); break;
    case FMT_RGB_RAW:         snprintf(buf,sz,"%d, %d, %d",r,g,b); break;
    case FMT_R:               snprintf(buf,sz,"%d",r); break;
    case FMT_G:               snprintf(buf,sz,"%d",g); break;
    case FMT_B:               snprintf(buf,sz,"%d",b); break;
    case FMT_R_FLOAT:         snprintf(buf,sz,"%.6f",r/255.0); break;
    case FMT_G_FLOAT:         snprintf(buf,sz,"%.6f",g/255.0); break;
    case FMT_B_FLOAT:         snprintf(buf,sz,"%.6f",b/255.0); break;
    case FMT_ARGB_INT:        snprintf(buf,sz,"%d",argb); break;
    case FMT_ARGB_HEX:        snprintf(buf,sz,"0x%08X",(unsigned)argb); break;
    case FMT_HSL: {
        float h2,s2,l2; rgb_to_hsl(r,g,b,&h2,&s2,&l2);
        snprintf(buf,sz,"hsl(%.1f, %.1f%%, %.1f%%)",h2,s2*100.0f,l2*100.0f);
        break; }
    case FMT_HCT_HUE:    { Hct hct=Hct_fromInt(argb); snprintf(buf,sz,"%.4f",hct.hue);    break; }
    case FMT_HCT_CHROMA: { Hct hct=Hct_fromInt(argb); snprintf(buf,sz,"%.4f",hct.chroma); break; }
    case FMT_HCT_TONE:   { Hct hct=Hct_fromInt(argb); snprintf(buf,sz,"%.4f",hct.tone);   break; }
    default:              snprintf(buf,sz,"#%02x%02x%02x",r,g,b); break;
    }
}
