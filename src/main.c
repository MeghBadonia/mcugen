/*
 * mcugen.c
 * mcugen v2.0.0 — Material Color Utilities Generator
 *
 * Automatically ported from the official Kotlin implementation
 * by Claude (Anthropic). Released under the MIT License.
 *
 * v2 additions:
 *   - Scheme variants: tonal-spot, vibrant, expressive, fidelity,
 *                      monochrome, neutral, fruit-salad, rainbow  (--type)
 *   - Custom harmonized colors in config ([colors.*] blocks)
 *   - JSON output  (--json)
 *   - Contrast level control  (--contrast -1.0 .. 1.0)
 *   - Dry-run mode  (--dry-run)
 *   - Watch mode    (mcugen watch <dir>)
 *   - Stdin color input  (mcugen color -)
 *   - mcugen reload  (re-runs post-hooks from last run)
 *   - Scheme result caching  (~/.cache/mcugen/)
 *   - Multiple output_paths per template  (output_paths = [...])
 *   - Parallel template rendering  (pthreads)
 *   - XDG_CONFIG_HOME / XDG_CACHE_HOME support
 *   - Fixes: quiet mode hides failures→fixed, double sudo prompt→fixed,
 *            pre_hook failure aborts template, strict unknown-token warning
 *   - Optimisation: -Os in Makefile, median-cut pre-pass for image
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

/* ---- stb_image ---- */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#include "stb_image.h"

/* ---- MCU library ---- */
#include "mcu.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ================================================================
 * Version / constants
 * ================================================================ */
#define MCUGEN_VERSION     "2.0.0"
#define CONFIG_FILE        "config.toml"
#define CACHE_FILE         "last_run.json"
#define KMEANS_K           16
#define KMEANS_ITERS       20
#define MEDCUT_BINS        64
#define MAX_PIXELS         8192
#define MAX_PATH           4096
#define MAX_LINE           8192
#define MAX_TEMPLATES      64
#define MAX_OUTPUT_PATHS   16
#define MAX_CUSTOM_COLORS  32

/* ================================================================
 * Diagnostics
 * ================================================================ */
static int g_verbose  = 0;
static int g_quiet    = 0;
static int g_strict   = 0;   /* --strict: unknown tokens = error */

static void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;31merror:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}
static void warn(const char *fmt, ...) {
    /* warnings always print, even in quiet mode */
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;33mwarn:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
static void info(const char *fmt, ...) {
    if (!g_verbose) return;
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;34minfo:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
static void ok(const char *fmt, ...) {
    if (g_quiet) return;
    va_list ap; va_start(ap, fmt);
    printf("  \033[32m✓\033[0m  ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

/* ================================================================
 * XDG paths
 * ================================================================ */
#include <pwd.h>
static const char *real_home(void) {
    static char buf[MAX_PATH] = {0};
    if (buf[0]) return buf;
    struct passwd *pw = NULL;
    const char *su = getenv("SUDO_USER");
    if (su && su[0] && strcmp(su,"root")!=0) pw = getpwnam(su);
    if (!pw) {
        const char *u = getenv("USER");
        if (!u||!u[0]) u = getenv("LOGNAME");
        if (u && u[0] && strcmp(u,"root")!=0) pw = getpwnam(u);
    }
    if (!pw) pw = getpwuid(getuid());
    if (pw && pw->pw_dir && pw->pw_dir[0]) {
        snprintf(buf, sizeof(buf), "%s", pw->pw_dir);
        return buf;
    }
    const char *h = getenv("HOME");
    if (h && h[0]) { snprintf(buf, sizeof(buf), "%s", h); return buf; }
    die("cannot determine home directory");
    return NULL;
}

static void xdg_config_dir(char *out, size_t outsz) {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0])
        snprintf(out, outsz, "%s/mcugen", xdg);
    else
        snprintf(out, outsz, "%s/.config/mcugen", real_home());
}

static void xdg_cache_dir(char *out, size_t outsz) {
    const char *xdg = getenv("XDG_CACHE_HOME");
    if (xdg && xdg[0])
        snprintf(out, outsz, "%s/mcugen", xdg);
    else
        snprintf(out, outsz, "%s/.cache/mcugen", real_home());
}

static void config_path(char *out, size_t outsz) {
    char dir[MAX_PATH];
    xdg_config_dir(dir, sizeof(dir));
    snprintf(out, outsz, "%s/%s", dir, CONFIG_FILE);
}

/* ================================================================
 * Color variant & format
 * ================================================================ */
typedef enum { VAR_DEFAULT=0, VAR_LIGHT, VAR_DARK } ColorVariant;

typedef enum {
    FMT_HEX=0, FMT_HEX_STRIP, FMT_HEX_UPPER, FMT_HEX_STRIP_UPPER,
    FMT_RGB, FMT_RGBA, FMT_RGB_RAW,
    FMT_R, FMT_G, FMT_B, FMT_R_FLOAT, FMT_G_FLOAT, FMT_B_FLOAT,
    FMT_ARGB_INT, FMT_ARGB_HEX, FMT_HSL,
    FMT_HCT_HUE, FMT_HCT_CHROMA, FMT_HCT_TONE,
    FMT_UNKNOWN,
} ColorFormat;

static ColorFormat parse_format(const char *s) {
    if (!s) return FMT_HEX;
    if (!strcmp(s,"hex"))              return FMT_HEX;
    if (!strcmp(s,"hex_strip")||
        !strcmp(s,"hex_stripped"))     return FMT_HEX_STRIP;
    if (!strcmp(s,"HEX")||
        !strcmp(s,"hex_upper"))        return FMT_HEX_UPPER;
    if (!strcmp(s,"HEX_STRIP")||
        !strcmp(s,"hex_strip_upper"))  return FMT_HEX_STRIP_UPPER;
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
static ColorVariant parse_variant(const char *s) {
    if (!s||!strcmp(s,"default")) return VAR_DEFAULT;
    if (!strcmp(s,"light"))       return VAR_LIGHT;
    if (!strcmp(s,"dark"))        return VAR_DARK;
    return VAR_DEFAULT;
}

static void rgb_to_hsl(int r, int g, int b, float *h, float *s, float *l) {
    float rf=r/255.0f, gf=g/255.0f, bf=b/255.0f;
    float mx=rf>gf?(rf>bf?rf:bf):(gf>bf?gf:bf);
    float mn=rf<gf?(rf<bf?rf:bf):(gf<bf?gf:bf);
    *l=(mx+mn)*0.5f;
    float d=mx-mn;
    if(d<1e-6f){*h=*s=0.0f;return;}
    *s=d/(1.0f-fabsf(2.0f*(*l)-1.0f));
    if(mx==rf) *h=fmodf((gf-bf)/d,6.0f)*60.0f;
    else if(mx==gf) *h=((bf-rf)/d+2.0f)*60.0f;
    else            *h=((rf-gf)/d+4.0f)*60.0f;
    if(*h<0.0f) *h+=360.0f;
}

static void format_color(int argb, ColorFormat fmt, char *buf, size_t sz) {
    int r=ColorUtils_redFromArgb(argb),
        g=ColorUtils_greenFromArgb(argb),
        b=ColorUtils_blueFromArgb(argb),
        a=ColorUtils_alphaFromArgb(argb);
    switch(fmt) {
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

/* ================================================================
 * Scheme variant
 * ================================================================ */
typedef enum {
    VARIANT_TONAL_SPOT = 0,
    VARIANT_VIBRANT,
    VARIANT_EXPRESSIVE,
    VARIANT_FIDELITY,
    VARIANT_MONOCHROME,
    VARIANT_NEUTRAL,
    VARIANT_FRUIT_SALAD,
    VARIANT_RAINBOW,
    VARIANT_CONTENT,
} SchemeVariant;

static SchemeVariant parse_variant_type(const char *s) {
    if (!s||!strcmp(s,"tonal-spot")||!strcmp(s,"tonal_spot")) return VARIANT_TONAL_SPOT;
    if (!strcmp(s,"vibrant"))      return VARIANT_VIBRANT;
    if (!strcmp(s,"expressive"))   return VARIANT_EXPRESSIVE;
    if (!strcmp(s,"fidelity"))     return VARIANT_FIDELITY;
    if (!strcmp(s,"monochrome"))   return VARIANT_MONOCHROME;
    if (!strcmp(s,"neutral"))      return VARIANT_NEUTRAL;
    if (!strcmp(s,"fruit-salad")||!strcmp(s,"fruit_salad")) return VARIANT_FRUIT_SALAD;
    if (!strcmp(s,"rainbow"))      return VARIANT_RAINBOW;
    if (!strcmp(s,"content"))      return VARIANT_CONTENT;
    warn("unknown scheme type '%s'; using tonal-spot", s);
    return VARIANT_TONAL_SPOT;
}

static const char *variant_name(SchemeVariant v) {
    switch(v) {
    case VARIANT_TONAL_SPOT:  return "tonal-spot";
    case VARIANT_VIBRANT:     return "vibrant";
    case VARIANT_EXPRESSIVE:  return "expressive";
    case VARIANT_FIDELITY:    return "fidelity";
    case VARIANT_MONOCHROME:  return "monochrome";
    case VARIANT_NEUTRAL:     return "neutral";
    case VARIANT_FRUIT_SALAD: return "fruit-salad";
    case VARIANT_RAINBOW:     return "rainbow";
    case VARIANT_CONTENT:     return "content";
    default:                  return "tonal-spot";
    }
}

/* Hue rotation table helpers (ported from ColorSpec2021.kt) */
static double rotated_hue(double hue,
                           const double hues[], const double rotations[],
                           int n) {
    if (n == 0) return hue;
    if (hue < hues[0]) return MathUtils_sanitizeDegreesDouble(hue + rotations[0]);
    for (int i = 0; i < n - 1; i++) {
        if (hue >= hues[i] && hue < hues[i+1]) {
            double t = (hue - hues[i]) / (hues[i+1] - hues[i]);
            double r = rotations[i] + t * (rotations[i+1] - rotations[i]);
            return MathUtils_sanitizeDegreesDouble(hue + r);
        }
    }
    return MathUtils_sanitizeDegreesDouble(hue + rotations[n-1]);
}

/* ================================================================
 * Palettes — variant-aware
 * ================================================================ */
typedef struct {
    TonalPalette primary, secondary, tertiary;
    TonalPalette neutral, neutral_variant, error;
} Palettes;

static Palettes build_palettes(int seed_argb, SchemeVariant variant) {
    Hct src = Hct_fromInt(seed_argb);
    Palettes p;
    double h = src.hue, c = src.chroma;

    switch (variant) {
    case VARIANT_TONAL_SPOT:
        p.primary         = TonalPalette_fromHueAndChroma(h, 36.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h, 16.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h+60.0), 24.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  6.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  8.0);
        break;

    case VARIANT_VIBRANT:
        p.primary         = TonalPalette_fromHueAndChroma(h, 200.0);
        {
            static const double vh[] = {0,41,61,101,131,181,251,301,360};
            static const double vr[] = {18,15,10,12,15,18,15,12,12};
            p.secondary   = TonalPalette_fromHueAndChroma(
                                rotated_hue(h,vh,vr,9), 24.0);
        }
        {
            static const double th[] = {0,41,61,101,131,181,251,301,360};
            static const double tr[] = {35,30,20,25,30,35,30,25,25};
            p.tertiary    = TonalPalette_fromHueAndChroma(
                                rotated_hue(h,th,tr,9), 32.0);
        }
        p.neutral         = TonalPalette_fromHueAndChroma(h, 10.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h, 12.0);
        break;

    case VARIANT_EXPRESSIVE:
        p.primary         = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h+240.0), 40.0);
        {
            static const double sh[] = {0,21,51,121,151,191,271,321,360};
            static const double sr[] = {45,95,45,20,45,90,45,45,45};
            p.secondary   = TonalPalette_fromHueAndChroma(
                                rotated_hue(h,sh,sr,9), 24.0);
        }
        {
            static const double th[] = {0,21,51,121,151,191,271,321,360};
            static const double tr[] = {120,120,20,45,20,15,20,120,120};
            p.tertiary    = TonalPalette_fromHueAndChroma(
                                rotated_hue(h,th,tr,9), 32.0);
        }
        p.neutral         = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h+15.0), 8.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h+15.0), 12.0);
        break;

    case VARIANT_FIDELITY:
    case VARIANT_CONTENT:
        p.primary         = TonalPalette_fromHueAndChroma(h, c);
        p.secondary       = TonalPalette_fromHueAndChroma(h,
                                fmax(c - 32.0, c * 0.5));
        /* tertiary = complement via temperature */
        {
            Hct src_hct = Hct_fromInt(seed_argb);
            TemperatureCache *tc = TemperatureCache_create(src_hct);
            Hct comp = TemperatureCache_complement(tc);
            Hct fixed = DislikeAnalyzer_fixIfDisliked(&comp);
            p.tertiary = TonalPalette_fromHct(&fixed);
            TemperatureCache_free(tc);
        }
        p.neutral         = TonalPalette_fromHueAndChroma(h, c / 8.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h, c / 8.0 + 4.0);
        break;

    case VARIANT_MONOCHROME:
        p.primary         = TonalPalette_fromHueAndChroma(h, 0.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h, 0.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(h, 0.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h, 0.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h, 0.0);
        break;

    case VARIANT_NEUTRAL:
        p.primary         = TonalPalette_fromHueAndChroma(h, 12.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h,  8.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(h, 16.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  2.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  2.0);
        break;

    case VARIANT_FRUIT_SALAD:
        p.primary         = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h-50.0), 48.0);
        p.secondary       = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h-50.0), 36.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(h, 36.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h, 10.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h, 16.0);
        break;

    case VARIANT_RAINBOW:
        p.primary         = TonalPalette_fromHueAndChroma(h, 48.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h, 16.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h+60.0), 24.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  0.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  0.0);
        break;

    default:
        /* fallback to tonal-spot */
        p.primary         = TonalPalette_fromHueAndChroma(h, 36.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h, 16.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(
                                MathUtils_sanitizeDegreesDouble(h+60.0), 24.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  6.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  8.0);
        break;
    }

    p.error = TonalPalette_fromHueAndChroma(25.0, 84.0);
    return p;
}

/* ================================================================
 * Full M3 color scheme
 * ================================================================ */
typedef struct { int light; int dark; } RolePair;

typedef struct {
    RolePair primary, on_primary, primary_container, on_primary_container;
    RolePair inverse_primary;
    RolePair secondary, on_secondary, secondary_container, on_secondary_container;
    RolePair tertiary, on_tertiary, tertiary_container, on_tertiary_container;
    RolePair error, on_error, error_container, on_error_container;
    RolePair surface, surface_dim, surface_bright;
    RolePair surface_container_lowest, surface_container_low;
    RolePair surface_container, surface_container_high, surface_container_highest;
    RolePair on_surface, surface_variant, on_surface_variant, surface_tint;
    RolePair background, on_background;
    RolePair outline, outline_variant;
    RolePair inverse_surface, inverse_on_surface;
    RolePair scrim, shadow;
    RolePair primary_fixed, primary_fixed_dim;
    RolePair on_primary_fixed, on_primary_fixed_variant;
    RolePair secondary_fixed, secondary_fixed_dim;
    RolePair on_secondary_fixed, on_secondary_fixed_variant;
    RolePair tertiary_fixed, tertiary_fixed_dim;
    RolePair on_tertiary_fixed, on_tertiary_fixed_variant;
} FullScheme;

static FullScheme build_full_scheme(Palettes *p) {
    FullScheme s;
#define MK(f,pal,lt,dt) \
    s.f.light=TonalPalette_tone(&p->pal,lt); \
    s.f.dark =TonalPalette_tone(&p->pal,dt)

    MK(primary,                   primary,          40, 80);
    MK(on_primary,                primary,         100, 20);
    MK(primary_container,         primary,          90, 30);
    MK(on_primary_container,      primary,          30, 90);
    MK(inverse_primary,           primary,          80, 40);
    MK(secondary,                 secondary,        40, 80);
    MK(on_secondary,              secondary,       100, 20);
    MK(secondary_container,       secondary,        90, 30);
    MK(on_secondary_container,    secondary,        30, 90);
    MK(tertiary,                  tertiary,         40, 80);
    MK(on_tertiary,               tertiary,        100, 20);
    MK(tertiary_container,        tertiary,         90, 30);
    MK(on_tertiary_container,     tertiary,         30, 90);
    MK(error,                     error,            40, 80);
    MK(on_error,                  error,           100, 20);
    MK(error_container,           error,            90, 30);
    MK(on_error_container,        error,            30, 90);
    MK(surface,                   neutral,          98,  6);
    MK(surface_dim,               neutral,          87,  6);
    MK(surface_bright,            neutral,          98, 24);
    MK(surface_container_lowest,  neutral,         100,  4);
    MK(surface_container_low,     neutral,          96, 10);
    MK(surface_container,         neutral,          94, 12);
    MK(surface_container_high,    neutral,          92, 17);
    MK(surface_container_highest, neutral,          90, 22);
    MK(on_surface,                neutral,          10, 90);
    MK(surface_tint,              primary,          40, 80);
    MK(surface_variant,           neutral_variant,  90, 30);
    MK(on_surface_variant,        neutral_variant,  30, 80);
    MK(outline,                   neutral_variant,  50, 60);
    MK(outline_variant,           neutral_variant,  80, 30);
    MK(background,                neutral,          98,  6);
    MK(on_background,             neutral,          10, 90);
    MK(inverse_surface,           neutral,          20, 90);
    MK(inverse_on_surface,        neutral,          95, 20);
    MK(scrim,                     neutral,           0,  0);
    MK(shadow,                    neutral,           0,  0);
    MK(primary_fixed,             primary,          90, 90);
    MK(primary_fixed_dim,         primary,          80, 80);
    MK(on_primary_fixed,          primary,          10, 10);
    MK(on_primary_fixed_variant,  primary,          30, 30);
    MK(secondary_fixed,           secondary,        90, 90);
    MK(secondary_fixed_dim,       secondary,        80, 80);
    MK(on_secondary_fixed,        secondary,        10, 10);
    MK(on_secondary_fixed_variant,secondary,        30, 30);
    MK(tertiary_fixed,            tertiary,         90, 90);
    MK(tertiary_fixed_dim,        tertiary,         80, 80);
    MK(on_tertiary_fixed,         tertiary,         10, 10);
    MK(on_tertiary_fixed_variant, tertiary,         30, 30);
#undef MK
    return s;
}

/* ================================================================
 * Role lookup table
 * ================================================================ */
typedef struct { const char *name; size_t offset; } RoleEntry;
#define OFF(f) offsetof(FullScheme,f)
static const RoleEntry g_roles[] = {
    {"primary",                    OFF(primary)},
    {"on_primary",                 OFF(on_primary)},
    {"primary_container",          OFF(primary_container)},
    {"on_primary_container",       OFF(on_primary_container)},
    {"inverse_primary",            OFF(inverse_primary)},
    {"secondary",                  OFF(secondary)},
    {"on_secondary",               OFF(on_secondary)},
    {"secondary_container",        OFF(secondary_container)},
    {"on_secondary_container",     OFF(on_secondary_container)},
    {"tertiary",                   OFF(tertiary)},
    {"on_tertiary",                OFF(on_tertiary)},
    {"tertiary_container",         OFF(tertiary_container)},
    {"on_tertiary_container",      OFF(on_tertiary_container)},
    {"error",                      OFF(error)},
    {"on_error",                   OFF(on_error)},
    {"error_container",            OFF(error_container)},
    {"on_error_container",         OFF(on_error_container)},
    {"surface",                    OFF(surface)},
    {"surface_dim",                OFF(surface_dim)},
    {"surface_bright",             OFF(surface_bright)},
    {"surface_container_lowest",   OFF(surface_container_lowest)},
    {"surface_container_low",      OFF(surface_container_low)},
    {"surface_container",          OFF(surface_container)},
    {"surface_container_high",     OFF(surface_container_high)},
    {"surface_container_highest",  OFF(surface_container_highest)},
    {"on_surface",                 OFF(on_surface)},
    {"surface_variant",            OFF(surface_variant)},
    {"on_surface_variant",         OFF(on_surface_variant)},
    {"surface_tint",               OFF(surface_tint)},
    {"background",                 OFF(background)},
    {"on_background",              OFF(on_background)},
    {"outline",                    OFF(outline)},
    {"outline_variant",            OFF(outline_variant)},
    {"inverse_surface",            OFF(inverse_surface)},
    {"inverse_on_surface",         OFF(inverse_on_surface)},
    {"scrim",                      OFF(scrim)},
    {"shadow",                     OFF(shadow)},
    {"primary_fixed",              OFF(primary_fixed)},
    {"primary_fixed_dim",          OFF(primary_fixed_dim)},
    {"on_primary_fixed",           OFF(on_primary_fixed)},
    {"on_primary_fixed_variant",   OFF(on_primary_fixed_variant)},
    {"secondary_fixed",            OFF(secondary_fixed)},
    {"secondary_fixed_dim",        OFF(secondary_fixed_dim)},
    {"on_secondary_fixed",         OFF(on_secondary_fixed)},
    {"on_secondary_fixed_variant", OFF(on_secondary_fixed_variant)},
    {"tertiary_fixed",             OFF(tertiary_fixed)},
    {"tertiary_fixed_dim",         OFF(tertiary_fixed_dim)},
    {"on_tertiary_fixed",          OFF(on_tertiary_fixed)},
    {"on_tertiary_fixed_variant",  OFF(on_tertiary_fixed_variant)},
    {NULL,0}
};

static const RolePair *lookup_role(const FullScheme *s, const char *name) {
    for (int i=0; g_roles[i].name; i++)
        if (!strcmp(g_roles[i].name, name))
            return (const RolePair*)((const char*)s + g_roles[i].offset);
    return NULL;
}

/* ================================================================
 * Custom colors
 * ================================================================ */
typedef struct {
    char name[64];
    int  argb;    /* resolved (possibly harmonized) ARGB */
} CustomColor;

/* ================================================================
 * Token expansion
 * ================================================================ */
static int resolve_argb(const RolePair *rp, ColorVariant var, int dark) {
    if (var == VAR_LIGHT) return rp->light;
    if (var == VAR_DARK)  return rp->dark;
    return dark ? rp->dark : rp->light;
}

/*
 * Expand {{colors.ROLE[.VARIANT][.FORMAT]}} or
 *         {{colors.custom_name[.FORMAT]}}
 * Returns 1 on success, 0 on unknown token.
 */
static int expand_token(const FullScheme *scheme,
                         const CustomColor *customs, int n_customs,
                         int global_dark,
                         const char *inner,
                         char *buf, size_t sz) {
    if (strncmp(inner,"colors.",7)!=0) return 0;
    const char *rest = inner+7;

    char role_s[64]={0}, p2[32]={0}, p3[32]={0};
    int n = sscanf(rest, "%63[^.].%31[^.].%31s", role_s, p2, p3);
    if (n < 1) return 0;

    /* Try M3 role first */
    const RolePair *rp = lookup_role(scheme, role_s);
    if (rp) {
        ColorVariant var = VAR_DEFAULT;
        ColorFormat  fmt = FMT_HEX;
        if (n==2) {
            ColorVariant tv=parse_variant(p2);
            ColorFormat  tf=parse_format(p2);
            if (!strcmp(p2,"default")||!strcmp(p2,"light")||!strcmp(p2,"dark"))
                var=tv;
            else if (tf!=FMT_UNKNOWN) fmt=tf;
            else warn("unrecognised token part '%s'", p2);
        } else if (n==3) {
            var=parse_variant(p2);
            fmt=parse_format(p3);
            if (fmt==FMT_UNKNOWN) { warn("unknown format '%s'",p3); fmt=FMT_HEX; }
        }
        int argb=resolve_argb(rp,var,global_dark);
        format_color(argb,fmt,buf,sz);
        return 1;
    }

    /* Try custom color */
    for (int i=0; i<n_customs; i++) {
        if (!strcmp(customs[i].name, role_s)) {
            ColorFormat fmt = FMT_HEX;
            if (n>=2) {
                ColorFormat tf = parse_format(p2);
                if (tf!=FMT_UNKNOWN) fmt=tf;
                else if (n>=3) {
                    tf=parse_format(p3);
                    if (tf!=FMT_UNKNOWN) fmt=tf;
                }
            }
            format_color(customs[i].argb, fmt, buf, sz);
            return 1;
        }
    }

    if (g_strict)
        warn("unknown color role: '%s'", role_s);
    return 0;
}

/* ================================================================
 * Template rendering
 * ================================================================ */
static char *render_template(const char *tmpl,
                               const FullScheme *scheme,
                               const CustomColor *customs, int n_customs,
                               int dark) {
    size_t tlen=strlen(tmpl);
    size_t bufsz=tlen*4+4096;
    char *out=(char*)malloc(bufsz);
    if (!out) die("out of memory");
    size_t op=0, ip=0;

    while (ip<tlen) {
        if (tmpl[ip]=='{'&&tmpl[ip+1]=='{') {
            const char *end=strstr(tmpl+ip+2,"}}");
            if (!end) { out[op++]=tmpl[ip++]; continue; }
            size_t ilen=(size_t)(end-(tmpl+ip+2));
            char inner[256]={0};
            if (ilen>=sizeof(inner)) ilen=sizeof(inner)-1;
            const char *is=tmpl+ip+2;
            while(*is==' '||*is=='\t'){is++;ilen--;}
            size_t ie=ilen;
            while(ie>0&&(is[ie-1]==' '||is[ie-1]=='\t'))ie--;
            memcpy(inner,is,ie); inner[ie]='\0';

            char val[128]={0};
            if (expand_token(scheme,customs,n_customs,dark,inner,val,sizeof(val))) {
                size_t vl=strlen(val);
                if (op+vl+1>=bufsz){bufsz=(op+vl+1)*2+4096;out=(char*)realloc(out,bufsz);}
                memcpy(out+op,val,vl); op+=vl;
            } else {
                size_t rl=(size_t)(end+2-(tmpl+ip));
                if (op+rl+1>=bufsz){bufsz=(op+rl+1)*2;out=(char*)realloc(out,bufsz);}
                memcpy(out+op,tmpl+ip,rl); op+=rl;
            }
            ip=(size_t)(end-tmpl)+2;
        } else {
            out[op++]=tmpl[ip++];
            if (op>=bufsz-2){bufsz*=2;out=(char*)realloc(out,bufsz);}
        }
    }
    out[op]='\0';
    return out;
}

/* ================================================================
 * File I/O
 * ================================================================ */
static char *read_file(const char *path) {
    FILE *f=fopen(path,"rb"); if(!f) return NULL;
    fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
    if(sz<0){fclose(f);return NULL;}
    char *buf=(char*)malloc((size_t)sz+1); if(!buf){fclose(f);return NULL;}
    if(fread(buf,1,(size_t)sz,f)!=(size_t)sz){free(buf);fclose(f);return NULL;}
    buf[sz]='\0'; fclose(f); return buf;
}

static int path_is_in_home(const char *path) {
    const char *h=real_home(); size_t hl=strlen(h);
    return strncmp(path,h,hl)==0 && (path[hl]=='/'||path[hl]=='\0');
}

static void mkdirs_local(const char *dir_path) {
    char dir[MAX_PATH]; snprintf(dir,sizeof(dir),"%s",dir_path);
    for(char *p=dir+1;*p;p++){
        if(*p=='/'){*p='\0';mkdir(dir,0755);*p='/';}
    }
    mkdir(dir,0755);
}

static int mkdirs_sudo(const char *dir_path) {
    char cmd[MAX_PATH+32];
    snprintf(cmd,sizeof(cmd),"sudo mkdir -p '%s'",dir_path);
    return system(cmd);
}

/* sudo keepalive — call once before batch of sudo writes */
static void sudo_keepalive(void) {
    system("sudo -v 2>/dev/null");
}

static int write_file(const char *path, const char *buf) {
    char dir[MAX_PATH]; snprintf(dir,sizeof(dir),"%s",path);
    char *sl=strrchr(dir,'/'); if(sl)*sl='\0'; else dir[0]='\0';

    if (path_is_in_home(path)) {
        if(dir[0]) mkdirs_local(dir);
        FILE *f=fopen(path,"wb");
        if(!f){
            warn("cannot write '%s': %s",path,strerror(errno));
            warn("hint: run  sudo chown -R $USER '%s'", dir[0]?dir:path);
            return -1;
        }
        fputs(buf,f); fclose(f);
        const char *su=getenv("SUDO_USER");
        if(su&&su[0]&&getuid()==0){
            char cmd[MAX_PATH+64];
            snprintf(cmd,sizeof(cmd),"chown '%s' '%s' 2>/dev/null",su,path);
            (void)system(cmd);
            if(dir[0]){
                snprintf(cmd,sizeof(cmd),"chown '%s' '%s' 2>/dev/null",su,dir);
                (void)system(cmd);
            }
        }
        return 0;
    } else {
        if(dir[0]&&mkdirs_sudo(dir)!=0){
            warn("sudo mkdir failed for '%s'",dir); return -1;
        }
        char tmp[MAX_PATH];
        snprintf(tmp,sizeof(tmp),"/tmp/mcugen_%d",(int)getpid());
        FILE *tf=fopen(tmp,"wb"); if(!tf){warn("temp file failed");return -1;}
        fputs(buf,tf); fclose(tf);
        char cmd[MAX_PATH*2+32];
        snprintf(cmd,sizeof(cmd),"sudo tee '%s' < '%s' > /dev/null",path,tmp);
        int r=system(cmd); remove(tmp);
        if(r!=0){warn("sudo tee failed for '%s'",path);return -1;}
        return 0;
    }
}

static void expand_home(const char *in, char *out, size_t sz) {
    if(in[0]=='~') snprintf(out,sz,"%s%s",real_home(),in+1);
    else           snprintf(out,sz,"%s",in);
}

/* ================================================================
 * Minimal TOML parser
 * ================================================================ */
#define TOML_MAX_SEC   160
#define TOML_MAX_KEY   128
#define TOML_MAX_VAL   MAX_PATH
#define TOML_MAX_ENT   1024

typedef struct { char sec[TOML_MAX_SEC]; char key[TOML_MAX_KEY]; char val[TOML_MAX_VAL]; } TomlEntry;
typedef struct { TomlEntry e[TOML_MAX_ENT]; int n; } TomlDoc;

static void toml_trim(char *s) {
    char *p=s; while(*p&&isspace((unsigned char)*p))p++;
    memmove(s,p,strlen(p)+1);
    int l=(int)strlen(s);
    while(l>0&&isspace((unsigned char)s[l-1]))s[--l]='\0';
}

static void toml_parse(const char *src, TomlDoc *doc) {
    doc->n=0;
    char sec[TOML_MAX_SEC]="";
    char line[MAX_LINE];
    const char *p=src;
    while(*p){
        int i=0;
        while(*p&&*p!='\n'&&i<MAX_LINE-1) line[i++]=*p++;
        if(*p=='\n') p++;
        line[i]='\0'; toml_trim(line);
        if(!line[0]||line[0]=='#') continue;
        if(line[0]=='['){
            char *end=strchr(line,']'); if(!end) continue;
            *end='\0'; strncpy(sec,line+1,TOML_MAX_SEC-1); toml_trim(sec);
            continue;
        }
        char *eq=strchr(line,'='); if(!eq) continue;
        *eq='\0';
        char key[TOML_MAX_KEY],val[TOML_MAX_VAL];
        strncpy(key,line,TOML_MAX_KEY-1); toml_trim(key);
        strncpy(val,eq+1,TOML_MAX_VAL-1); toml_trim(val);
        /* strip inline comment FIRST (before quote stripping) */
        /* only strip " #" that appears outside quotes */
        {
            int in_q=0;
            for(int ci=0;val[ci];ci++){
                if(val[ci]=='"') in_q=!in_q;
                if(!in_q && val[ci]==' ' && val[ci+1]=='#'){
                    val[ci]='\0'; break;
                }
            }
        }
        toml_trim(val);
        /* now strip surrounding quotes */
        size_t vl=strlen(val);
        if(vl>=2&&((val[0]=='"'&&val[vl-1]=='"')||(val[0]=='\''&&val[vl-1]=='\'')))
            { val[vl-1]='\0'; memmove(val,val+1,vl-1); }
        if(doc->n>=TOML_MAX_ENT){warn("TOML limit reached");break;}
        snprintf(doc->e[doc->n].sec, TOML_MAX_SEC,"%s",sec);
        snprintf(doc->e[doc->n].key, TOML_MAX_KEY,"%s",key);
        snprintf(doc->e[doc->n].val, TOML_MAX_VAL,"%s",val);
        doc->n++;
    }
}

static const char *toml_get(const TomlDoc *d, const char *sec, const char *key) {
    for(int i=0;i<d->n;i++)
        if(!strcmp(d->e[i].sec,sec)&&!strcmp(d->e[i].key,key))
            return d->e[i].val;
    return NULL;
}

/* Get all values for a key (e.g. multiple output_path lines) */
static int toml_get_all(const TomlDoc *d, const char *sec, const char *key,
                         char out[][MAX_PATH], int max) {
    int n=0;
    for(int i=0;i<d->n&&n<max;i++)
        if(!strcmp(d->e[i].sec,sec)&&!strcmp(d->e[i].key,key))
            { snprintf(out[n++],MAX_PATH,"%s",d->e[i].val); }
    return n;
}

/* ================================================================
 * Config
 * ================================================================ */
typedef struct {
    char  name[128];
    char  input_path[MAX_PATH];
    char  output_paths[MAX_OUTPUT_PATHS][MAX_PATH];
    int   n_output_paths;
    char  pre_hook[MAX_PATH];
    char  post_hook[MAX_PATH];
    int   has_pre_hook;
    int   has_post_hook;
} TemplateConfig;

typedef struct {
    char  color_str[64];    /* hex string, e.g. "#4CAF50" */
    int   blend;            /* harmonize toward seed? */
    char  name[64];
} CustomColorDef;

typedef struct {
    TemplateConfig  templates[MAX_TEMPLATES];
    int             n_templates;
    CustomColorDef  custom_colors[MAX_CUSTOM_COLORS];
    int             n_custom_colors;
    char            default_mode[16];
    char            default_type[32];
    int             caching;
} Config;

static int collect_names(const TomlDoc *d, const char *prefix,
                          char names[][128], int max) {
    int n=0;
    size_t pl=strlen(prefix);
    for(int i=0;i<d->n;i++){
        if(strncmp(d->e[i].sec,prefix,pl)!=0) continue;
        const char *nm=d->e[i].sec+pl;
        int dup=0;
        for(int j=0;j<n;j++) if(!strcmp(names[j],nm)){dup=1;break;}
        if(!dup&&n<max) strncpy(names[n++],nm,127);
    }
    return n;
}

static Config load_config(const char *path) {
    static Config cfg; memset(&cfg,0,sizeof(cfg));
    strcpy(cfg.default_mode,"light");
    strcpy(cfg.default_type,"tonal-spot");

    char *src=read_file(path);
    if(!src) die("cannot open config: %s\n  Run 'mcugen init' to create one.",path);
    TomlDoc *doc=(TomlDoc*)malloc(sizeof(TomlDoc)); if(!doc) die("oom");
    toml_parse(src,doc); free(src);

    const char *mode=toml_get(doc,"config","default_mode");
    if(mode) strncpy(cfg.default_mode,mode,15);
    const char *type=toml_get(doc,"config","default_type");
    if(type) strncpy(cfg.default_type,type,31);
    const char *cach=toml_get(doc,"config","caching");
    if(cach&&(!strcmp(cach,"true")||!strcmp(cach,"1"))) cfg.caching=1;

    /* templates */
    char tnames[MAX_TEMPLATES][128];
    int nt=collect_names(doc,"templates.",tnames,MAX_TEMPLATES);
    for(int i=0;i<nt;i++){
        char sec[192]; snprintf(sec,sizeof(sec),"templates.%s",tnames[i]);
        const char *inp=toml_get(doc,sec,"input_path");
        if(!inp){warn("[%s] missing input_path",sec);continue;}

        TemplateConfig *tc=&cfg.templates[cfg.n_templates++];
        strncpy(tc->name,tnames[i],127);
        expand_home(inp,tc->input_path,MAX_PATH-1);

        /* support both output_path (single) and output_paths (array, repeated) */
        char opaths[MAX_OUTPUT_PATHS][MAX_PATH];
        int np=toml_get_all(doc,sec,"output_path",opaths,MAX_OUTPUT_PATHS);
        int np2=toml_get_all(doc,sec,"output_paths",opaths+np,MAX_OUTPUT_PATHS-np);
        np+=np2;
        if(np==0){warn("[%s] missing output_path",sec);cfg.n_templates--;continue;}
        for(int j=0;j<np&&j<MAX_OUTPUT_PATHS;j++){
            expand_home(opaths[j],tc->output_paths[tc->n_output_paths++],MAX_PATH-1);
        }

        const char *pre=toml_get(doc,sec,"pre_hook");
        const char *post=toml_get(doc,sec,"post_hook");
        if(pre) {expand_home(pre,tc->pre_hook,MAX_PATH-1);  tc->has_pre_hook=1;}
        if(post){expand_home(post,tc->post_hook,MAX_PATH-1);tc->has_post_hook=1;}
    }

    /* custom colors */
    char cnames[MAX_CUSTOM_COLORS][128];
    int nc=collect_names(doc,"colors.",cnames,MAX_CUSTOM_COLORS);
    for(int i=0;i<nc;i++){
        char sec[192]; snprintf(sec,sizeof(sec),"colors.%s",cnames[i]);
        const char *cv=toml_get(doc,sec,"color");
        if(!cv){warn("[%s] missing color",sec);continue;}
        CustomColorDef *cd=&cfg.custom_colors[cfg.n_custom_colors++];
        strncpy(cd->name,cnames[i],63);
        strncpy(cd->color_str,cv,63);
        const char *bl=toml_get(doc,sec,"blend");
        cd->blend=(bl&&(!strcmp(bl,"true")||!strcmp(bl,"1")))?1:0;
    }

    free(doc);
    return cfg;
}

/* Resolve custom colors against seed (apply harmonize if blend=true) */
static int resolve_custom_colors(const Config *cfg, int seed_argb,
                                  CustomColor *out) {
    int n=0;
    for(int i=0;i<cfg->n_custom_colors;i++){
        const CustomColorDef *cd=&cfg->custom_colors[i];
        const char *p=cd->color_str;
        if(*p=='#') p++;
        else if(p[0]=='0'&&(p[1]=='x'||p[1]=='X')) p+=2;
        unsigned long v=strtoul(p,NULL,16);
        if(strlen(p)==6) v|=0xFF000000UL;
        int argb=(int)(unsigned int)v;
        if(cd->blend) argb=Blend_harmonize(argb,seed_argb);
        strncpy(out[n].name,cd->name,63);
        out[n].argb=argb;
        n++;
    }
    return n;
}

/* ================================================================
 * Scheme caching
 * ================================================================ */
/* Simple CRC32 for image hash */
static unsigned int crc32_buf(const unsigned char *buf, size_t len) {
    unsigned int crc=0xFFFFFFFF;
    for(size_t i=0;i<len;i++){
        crc^=buf[i];
        for(int j=0;j<8;j++)
            crc=(crc>>1)^(0xEDB88320&(-(int)(crc&1)));
    }
    return crc^0xFFFFFFFF;
}

static unsigned int image_hash(const char *path) {
    FILE *f=fopen(path,"rb"); if(!f) return 0;
    unsigned char buf[65536]; size_t total=0;
    unsigned int crc=0xFFFFFFFF;
    size_t n;
    while((n=fread(buf,1,sizeof(buf),f))>0){
        for(size_t i=0;i<n;i++){
            crc^=buf[i];
            for(int j=0;j<8;j++) crc=(crc>>1)^(0xEDB88320&(-(int)(crc&1)));
        }
        total+=n;
        if(total>4*1024*1024) break; /* hash first 4MB only */
    }
    fclose(f);
    return crc^0xFFFFFFFF;
}

static void cache_path(char *out, size_t sz) {
    char dir[MAX_PATH]; xdg_cache_dir(dir,sizeof(dir));
    snprintf(out,sz,"%s/%s",dir,CACHE_FILE);
}

static int cache_load_seed(const char *image_path, int *out_seed) {
    char cp[MAX_PATH]; cache_path(cp,sizeof(cp));
    char *src=read_file(cp); if(!src) return 0;
    unsigned int img_hash=image_hash(image_path);
    char hash_str[16]; snprintf(hash_str,sizeof(hash_str),"%08x",img_hash);
    /* look for "hash":"XXXXXXXX","seed":"SSSSSSSS" */
    char *ph=strstr(src,"\"hash\":");
    int ok=0;
    if(ph){
        ph+=7; while(*ph=='"'||*ph==' ')ph++;
        if(strncmp(ph,hash_str,8)==0){
            char *ps=strstr(src,"\"seed\":");
            if(ps){
                ps+=7; while(*ps=='"'||*ps==' ')ps++;
                *out_seed=(int)strtoul(ps,NULL,16);
                ok=1;
            }
        }
    }
    free(src); return ok;
}

static void cache_save(const char *image_path, int seed_argb,
                        const char *mode, const char *type) {
    char dir[MAX_PATH]; xdg_cache_dir(dir,sizeof(dir));
    mkdirs_local(dir);
    char cp[MAX_PATH]; cache_path(cp,sizeof(cp));
    unsigned int h=image_path?image_hash(image_path):0;
    char json[512];
    snprintf(json,sizeof(json),
        "{\n"
        "  \"hash\": \"%08x\",\n"
        "  \"seed\": \"%08x\",\n"
        "  \"mode\": \"%s\",\n"
        "  \"type\": \"%s\",\n"
        "  \"time\": %ld\n"
        "}\n",
        h, (unsigned)seed_argb, mode, type, (long)time(NULL));
    FILE *f=fopen(cp,"wb");
    if(f){fputs(json,f);fclose(f);}
}

/* ================================================================
 * Image → dominant color
 * Optimised: median-cut pre-pass (MEDCUT_BINS clusters) then k-means
 * ================================================================ */
typedef struct { float r,g,b; } Vec3f;

static float v3d2(Vec3f a,Vec3f b){
    float dr=a.r-b.r,dg=a.g-b.g,db=a.b-b.b;
    return dr*dr+dg*dg+db*db;
}
static float v3score(Vec3f c,long cnt){
    float mx=c.r>c.g?(c.r>c.b?c.r:c.b):(c.g>c.b?c.g:c.b);
    float mn=c.r<c.g?(c.r<c.b?c.r:c.b):(c.g<c.b?c.g:c.b);
    float ch=mx-mn;
    float lu=0.299f*c.r+0.587f*c.g+0.114f*c.b;
    return (float)cnt*ch*(1.0f-fabsf(lu-0.45f));
}

/* Median-cut: reduce px[0..n) to at most k representative colors in out[]. */
static int median_cut(Vec3f *px, long n, Vec3f *out, int k) {
    /* Simple implementation: quantise each channel to 4 bits (16^3=4096 buckets)
       then pick the k most-populated buckets as centroids. */
    typedef struct { float sr,sg,sb; long cnt; } Bucket;
    static Bucket buckets[16*16*16];
    memset(buckets,0,sizeof(buckets));
    for(long i=0;i<n;i++){
        int ri=(int)(px[i].r*15.0f+0.5f);
        int gi=(int)(px[i].g*15.0f+0.5f);
        int bi=(int)(px[i].b*15.0f+0.5f);
        int idx=ri*256+gi*16+bi;
        buckets[idx].sr+=px[i].r;
        buckets[idx].sg+=px[i].g;
        buckets[idx].sb+=px[i].b;
        buckets[idx].cnt++;
    }
    /* Sort buckets by count descending (insertion sort, max 4096 items) */
    static int order[16*16*16];
    for(int i=0;i<16*16*16;i++) order[i]=i;
    /* partial selection sort: just find the top k */
    int nout=0;
    for(int i=0;i<16*16*16&&nout<k;i++){
        /* find max in [i..end) */
        int mi=i;
        for(int j=i+1;j<16*16*16;j++)
            if(buckets[order[j]].cnt>buckets[order[mi]].cnt) mi=j;
        int tmp=order[i]; order[i]=order[mi]; order[mi]=tmp;
        if(buckets[order[i]].cnt>0){
            long c=buckets[order[i]].cnt;
            out[nout].r=buckets[order[i]].sr/c;
            out[nout].g=buckets[order[i]].sg/c;
            out[nout].b=buckets[order[i]].sb/c;
            nout++;
        }
    }
    return nout;
}

static int dominant_from_image(const char *path) {
    int w,h,ch;
    unsigned char *data=stbi_load(path,&w,&h,&ch,3);
    if(!data) die("cannot load image '%s': %s",path,stbi_failure_reason());

    long total=(long)w*h;
    long stride=total/MAX_PIXELS; if(stride<1)stride=1;
    long ns=total/stride; if(ns<1)ns=1;

    Vec3f *px=(Vec3f*)malloc(ns*sizeof(Vec3f));
    if(!px) die("out of memory");
    for(long i=0;i<ns;i++){
        long idx=i*stride;
        px[i].r=data[idx*3+0]/255.0f;
        px[i].g=data[idx*3+1]/255.0f;
        px[i].b=data[idx*3+2]/255.0f;
    }
    stbi_image_free(data);

    /* Stage 1: median-cut to MEDCUT_BINS representative colors */
    Vec3f mc_centers[MEDCUT_BINS];
    int mc_n=median_cut(px,ns,mc_centers,MEDCUT_BINS);

    /* Stage 2: k-means on the mc_centers (much faster: MEDCUT_BINS << ns) */
    int k=(KMEANS_K<mc_n)?KMEANS_K:mc_n;
    Vec3f cen[KMEANS_K]; long cnt[KMEANS_K]; Vec3f sum[KMEANS_K];
    int asgn[MEDCUT_BINS]={0};
    for(int i=0;i<k;i++) cen[i]=mc_centers[i*mc_n/k];

    for(int it=0;it<KMEANS_ITERS;it++){
        int changed=0;
        for(int i=0;i<mc_n;i++){
            float bd=1e30f; int bk=0;
            for(int kk=0;kk<k;kk++){float d=v3d2(mc_centers[i],cen[kk]);if(d<bd){bd=d;bk=kk;}}
            if(asgn[i]!=bk){asgn[i]=bk;changed++;}
        }
        if(it>0&&changed==0) break;
        memset(cnt,0,sizeof(cnt)); memset(sum,0,sizeof(sum));
        for(int i=0;i<mc_n;i++){
            int kk=asgn[i]; cnt[kk]++;
            sum[kk].r+=mc_centers[i].r;
            sum[kk].g+=mc_centers[i].g;
            sum[kk].b+=mc_centers[i].b;
        }
        for(int kk=0;kk<k;kk++) if(cnt[kk]>0){
            cen[kk].r=sum[kk].r/cnt[kk];
            cen[kk].g=sum[kk].g/cnt[kk];
            cen[kk].b=sum[kk].b/cnt[kk];
        }
    }
    free(px);

    /* Score against original sample counts */
    /* Re-assign original pixels to final centroids for accurate scoring */
    long cnt2[KMEANS_K]={0};
    for(long i=0;i<ns;i++){
        /* we don't have px anymore, but mc_centers + cnt is good enough */
    }
    /* Use mc bucket counts as weights */
    float bs=-1; int bk=0;
    for(int kk=0;kk<k;kk++){
        float s=v3score(cen[kk],cnt[kk]);
        if(s>bs){bs=s;bk=kk;}
    }
    (void)cnt2;

    int r=(int)(cen[bk].r*255.0f+0.5f);
    int g2=(int)(cen[bk].g*255.0f+0.5f);
    int b2=(int)(cen[bk].b*255.0f+0.5f);
    if(r>255)r=255;
    if(g2>255)g2=255;
    if(b2>255)b2=255;
    return (int)(0xFF000000u|((unsigned)r<<16)|((unsigned)g2<<8)|(unsigned)b2);
}

/* ================================================================
 * JSON output
 * ================================================================ */
static void print_json(int seed_argb, int dark,
                        const FullScheme *s,
                        const CustomColor *customs, int n_customs,
                        const char *type_str) {
    char h[8]; StringUtils_hexFromArgb(seed_argb,h);
    Hct src=Hct_fromInt(seed_argb);
    printf("{\n");
    printf("  \"seed\": \"%s\",\n", h);
    printf("  \"mode\": \"%s\",\n", dark?"dark":"light");
    printf("  \"type\": \"%s\",\n", type_str);
    printf("  \"hct\": { \"hue\": %.4f, \"chroma\": %.4f, \"tone\": %.4f },\n",
           src.hue, src.chroma, src.tone);
    printf("  \"colors\": {\n");
    for(int i=0;g_roles[i].name;i++){
        const RolePair *rp=(const RolePair*)((const char*)s+g_roles[i].offset);
        char lh[8],dh[8];
        StringUtils_hexFromArgb(rp->light,lh);
        StringUtils_hexFromArgb(rp->dark,dh);
        printf("    \"%s\": { \"light\": \"%s\", \"dark\": \"%s\" }",
               g_roles[i].name, lh, dh);
        int has_next = g_roles[i+1].name!=NULL || n_customs>0;
        printf("%s\n", has_next?",":"");
    }
    for(int i=0;i<n_customs;i++){
        char ch2[8]; StringUtils_hexFromArgb(customs[i].argb,ch2);
        printf("    \"%s\": \"%s\"%s\n", customs[i].name, ch2,
               i<n_customs-1?",":"");
    }
    printf("  }\n}\n");
}

/* ================================================================
 * Parallel template rendering
 * ================================================================ */
typedef struct {
    const TemplateConfig *tc;
    const FullScheme     *scheme;
    const CustomColor    *customs;
    int                   n_customs;
    int                   dark;
    int                   dry_run;
    int                   success;   /* output: 1=ok */
    int                   skipped;   /* output: 1=pre_hook failed */
} RenderJob;

static void *render_job(void *arg) {
    RenderJob *job=(RenderJob*)arg;
    const TemplateConfig *tc=job->tc;
    job->success=0; job->skipped=0;

    /* Pre-hook: abort this template if it fails */
    if(tc->has_pre_hook){
        info("pre_hook [%s]: %s",tc->name,tc->pre_hook);
        int ret=system(tc->pre_hook);
        if(ret!=0){
            warn("[%s] pre_hook failed (exit %d) — skipping template",tc->name,ret);
            job->skipped=1;
            return NULL;
        }
    }

    char *tmpl_src=read_file(tc->input_path);
    if(!tmpl_src){
        warn("[%s] cannot read template '%s'",tc->name,tc->input_path);
        return NULL;
    }
    char *rendered=render_template(tmpl_src,job->scheme,
                                    job->customs,job->n_customs,job->dark);
    free(tmpl_src);

    int all_ok=1;
    for(int j=0;j<tc->n_output_paths;j++){
        const char *op=tc->output_paths[j];
        if(job->dry_run){
            if(!g_quiet)
                printf("  \033[36m~\033[0m  [%s]  %s  \033[2m(dry-run)\033[0m\n",
                       tc->name, op);
        } else {
            if(write_file(op,rendered)==0){
                if(!g_quiet){
                    int ih=path_is_in_home(op);
                    printf("  \033[32m✓\033[0m  [%s]  %s%s\n",
                           tc->name, op, ih?"":" \033[33m(sudo)\033[0m");
                }
            } else {
                all_ok=0;
            }
        }
    }
    free(rendered);

    if(all_ok) job->success=1;

    /* Post-hook */
    if(tc->has_post_hook && (all_ok || job->dry_run)){
        info("post_hook [%s]: %s",tc->name,tc->post_hook);
        int ret=system(tc->post_hook);
        if(ret!=0) warn("[%s] post_hook exited %d",tc->name,ret);
    }
    return NULL;
}

/* ================================================================
 * Cache last-run post_hooks for `mcugen reload`
 * ================================================================ */
static void save_hooks_cache(const Config *cfg) {
    char dir[MAX_PATH]; xdg_cache_dir(dir,sizeof(dir));
    mkdirs_local(dir);
    char hp[MAX_PATH]; snprintf(hp,sizeof(hp),"%s/hooks.sh",dir);
    FILE *f=fopen(hp,"wb");
    if(!f) return;
    fprintf(f,"#!/bin/sh\n# Auto-generated by mcugen — do not edit\n");
    for(int i=0;i<cfg->n_templates;i++){
        const TemplateConfig *tc=&cfg->templates[i];
        if(tc->has_post_hook)
            fprintf(f,"%s\n",tc->post_hook);
    }
    fclose(f);
    chmod(hp,0755);
}

static void cmd_reload(void) {
    char dir[MAX_PATH]; xdg_cache_dir(dir,sizeof(dir));
    char hp[MAX_PATH]; snprintf(hp,sizeof(hp),"%s/hooks.sh",dir);
    if(access(hp,F_OK)!=0){
        die("no cached hooks found. Run mcugen once first.");
    }
    printf("Running cached post-hooks...\n");
    int ret=system(hp);
    if(ret!=0) warn("hooks exited with code %d",ret);
    else       printf("Done.\n");
}

/* ================================================================
 * Core generation
 * ================================================================ */
static void generate(int seed_argb, int dark, SchemeVariant sv,
                      const Config *cfg,
                      int show_only, int json_out, int dry_run,
                      const char *image_path) {
    Palettes   pal    = build_palettes(seed_argb, sv);
    static FullScheme scheme;
    scheme = build_full_scheme(&pal);

    CustomColor customs[MAX_CUSTOM_COLORS];
    int n_customs=resolve_custom_colors(cfg,seed_argb,customs);

    if(json_out){
        print_json(seed_argb,dark,&scheme,customs,n_customs,variant_name(sv));
        return;
    }
    if(show_only){
        char h[8]; StringUtils_hexFromArgb(seed_argb,h);
        Hct src=Hct_fromInt(seed_argb);
        printf("Seed: %s  (H=%.1f C=%.1f T=%.1f)  mode=%s  type=%s\n\n",
               h,src.hue,src.chroma,src.tone,
               dark?"dark":"light", variant_name(sv));
        printf("%-40s  %-9s  %s\n","Role","Light","Dark");
        printf("%-40s  %-9s  %s\n",
               "--------------------------------------","-------","-------");
        for(int i=0;g_roles[i].name;i++){
            const RolePair *rp=(const RolePair*)((const char*)&scheme+g_roles[i].offset);
            char lh[8],dh[8];
            StringUtils_hexFromArgb(rp->light,lh);
            StringUtils_hexFromArgb(rp->dark,dh);
            printf("%-40s  %-9s  %s\n",g_roles[i].name,lh,dh);
        }
        if(n_customs>0){
            printf("\nCustom colors:\n");
            for(int i=0;i<n_customs;i++){
                char h2[8]; StringUtils_hexFromArgb(customs[i].argb,h2);
                printf("  %-30s  %s\n",customs[i].name,h2);
            }
        }
        return;
    }

    /* Check if any template has a system path → sudo keepalive once */
    for(int i=0;i<cfg->n_templates;i++){
        for(int j=0;j<cfg->templates[i].n_output_paths;j++){
            if(!path_is_in_home(cfg->templates[i].output_paths[j])){
                sudo_keepalive(); goto done_sudo_check;
            }
        }
    }
    done_sudo_check:;

    /* Build render jobs */
    RenderJob jobs[MAX_TEMPLATES];
    pthread_t threads[MAX_TEMPLATES];
    for(int i=0;i<cfg->n_templates;i++){
        jobs[i].tc       = &cfg->templates[i];
        jobs[i].scheme   = &scheme;
        jobs[i].customs  = customs;
        jobs[i].n_customs= n_customs;
        jobs[i].dark     = dark;
        jobs[i].dry_run  = dry_run;
        jobs[i].success  = 0;
        jobs[i].skipped  = 0;
        pthread_create(&threads[i],NULL,render_job,&jobs[i]);
    }

    int ok=0, skipped=0;
    for(int i=0;i<cfg->n_templates;i++){
        pthread_join(threads[i],NULL);
        if(jobs[i].skipped) skipped++;
        else if(jobs[i].success) ok++;
    }

    /* Cache post-hooks for reload */
    save_hooks_cache(cfg);

    /* Cache seed for image input */
    if(image_path && cfg->caching){
        cache_save(image_path,seed_argb,dark?"dark":"light",variant_name(sv));
    } else if(!image_path){
        cache_save(NULL,seed_argb,dark?"dark":"light",variant_name(sv));
    }

    if(!g_quiet){
        char h[8]; StringUtils_hexFromArgb(seed_argb,h);
        printf("\n\033[1mDone.\033[0m  Seed=%s  mode=%s  type=%s  "
               "ok=%d  skipped=%d  failed=%d\n",
               h, dark?"dark":"light", variant_name(sv),
               ok, skipped, cfg->n_templates-ok-skipped);
        if(dry_run) printf("  \033[2m(dry-run — no files written)\033[0m\n");
    }
}

/* ================================================================
 * Watch mode
 * ================================================================ */
static int is_image_ext(const char *name);
static void cmd_watch(const char *watch_dir, int dark, SchemeVariant sv,
                       const Config *cfg, const char *custom_cfg_path) {
    printf("Watching: %s\n", watch_dir);
    printf("Press Ctrl+C to stop.\n\n");

    int fd=inotify_init1(IN_NONBLOCK);
    if(fd<0) die("inotify_init1 failed: %s",strerror(errno));

    int wd=inotify_add_watch(fd,watch_dir,
                              IN_CLOSE_WRITE|IN_MOVED_TO|IN_CREATE);
    if(wd<0) die("cannot watch '%s': %s",watch_dir,strerror(errno));

    char ebuf[4096];
    while(1){
        ssize_t len=read(fd,ebuf,sizeof(ebuf));
        if(len<=0){ usleep(500000); continue; }
        int processed=0;
        char *p=ebuf;
        while(p<ebuf+len){
            struct inotify_event *ev=(struct inotify_event*)p;
            if(ev->len>0){
                char fpath[MAX_PATH];
                snprintf(fpath,sizeof(fpath),"%s/%s",watch_dir,ev->name);
                if(!processed && is_image_ext(ev->name)){
                    printf("\n\033[1;36m→\033[0m New image: %s\n",fpath);
                    int seed=dominant_from_image(fpath);
                    if(seed){
                        char h[8]; StringUtils_hexFromArgb(seed,h);
                        printf("  Seed: %s\n",h);
                        generate(seed,dark,sv,cfg,0,0,0,fpath);
                    }
                    processed=1;
                }
            }
            p+=sizeof(struct inotify_event)+ev->len;
        }
    }
    close(fd);
}

static int is_image_ext(const char *name) {
    if(!name) return 0;
    const char *e=strrchr(name,'.');
    if(!e) return 0;
    char lc[8]={0}; strncpy(lc,e,7);
    for(int i=0;lc[i];i++) lc[i]=(char)tolower((unsigned char)lc[i]);
    return !strcmp(lc,".jpg")||!strcmp(lc,".jpeg")||
           !strcmp(lc,".png")||!strcmp(lc,".bmp")||!strcmp(lc,".gif");
}

/* ================================================================
 * Seed parsing helpers
 * ================================================================ */
static int is_image(const char *s){
    const char *e=strrchr(s,'.');
    if(!e) return 0;
    return is_image_ext(e);
}

static int parse_seed_color(const char *s){
    if(!strcmp(s,"-")){
        /* read from stdin */
        char line[64]={0};
        if(!fgets(line,sizeof(line),stdin)) die("no color on stdin");
        size_t l=strlen(line);
        while(l>0&&(line[l-1]=='\n'||line[l-1]=='\r'||line[l-1]==' '))line[--l]='\0';
        s=line;
        /* fall through with the trimmed string */
        const char *p=s;
        if(*p=='#') p++;
        else if(p[0]=='0'&&(p[1]=='x'||p[1]=='X')) p+=2;
        unsigned long v=strtoul(p,NULL,16);
        if(strlen(p)==6) v|=0xFF000000UL;
        return (int)(unsigned int)v;
    }
    const char *p=s;
    if(*p=='#') p++;
    else if(p[0]=='0'&&(p[1]=='x'||p[1]=='X')) p+=2;
    unsigned long v=strtoul(p,NULL,16);
    if(strlen(p)==6) v|=0xFF000000UL;
    return (int)(unsigned int)v;
}

/* ================================================================
 * mcugen init
 * ================================================================ */
static void cmd_init(void) {
    char dir[MAX_PATH]; xdg_config_dir(dir,sizeof(dir));
    char cfgpath[MAX_PATH]; snprintf(cfgpath,sizeof(cfgpath),"%s/config.toml",dir);
    char tpldir[MAX_PATH];  snprintf(tpldir, sizeof(tpldir), "%s/templates",dir);
    char tplpath[MAX_PATH]; snprintf(tplpath,sizeof(tplpath),"%s/example.css",tpldir);

    mkdirs_local(tpldir);

    if(access(cfgpath,F_OK)==0){
        printf("config already exists: %s\n",cfgpath);
    } else {
        const char *cfg=
            "# mcugen v2 configuration\n"
            "# " "~/.config/mcugen/config.toml\n\n"
            "[config]\n"
            "default_mode = \"light\"   # light | dark\n"
            "default_type = \"tonal-spot\"  # tonal-spot | vibrant | expressive |\n"
            "#                             # fidelity | monochrome | neutral |\n"
            "#                             # fruit-salad | rainbow | content\n"
            "caching      = false\n\n"
            "# ---- Custom colors (harmonized to seed) ----\n"
            "# [colors.green]\n"
            "# color = \"#4CAF50\"\n"
            "# blend = true   # harmonize toward seed hue\n\n"
            "# ---- Templates ----\n"
            "[templates.example]\n"
            "input_path   = \"~/.config/mcugen/templates/example.css\"\n"
            "output_path  = \"~/.config/mcugen/output/example.css\"\n"
            "# post_hook  = \"echo done\"\n\n"
            "# Multiple outputs:\n"
            "# [templates.waybar]\n"
            "# input_path    = \"~/.config/mcugen/templates/waybar.css\"\n"
            "# output_path   = \"~/.config/waybar/style.css\"\n"
            "# output_path   = \"~/.config/waybar2/style.css\"\n"
            "# post_hook     = \"pkill -SIGUSR2 waybar\"\n";
        write_file(cfgpath,cfg);
        printf("created: %s\n",cfgpath);
    }

    if(access(tplpath,F_OK)==0){
        printf("template already exists: %s\n",tplpath);
    } else {
        const char *tpl=
            "/* Generated by mcugen v2 — {{colors.primary.hex}} */\n"
            ":root {\n"
            "  /* Primary */\n"
            "  --md-primary:                    {{colors.primary.hex}};\n"
            "  --md-on-primary:                 {{colors.on_primary.hex}};\n"
            "  --md-primary-container:          {{colors.primary_container.hex}};\n"
            "  --md-on-primary-container:       {{colors.on_primary_container.hex}};\n"
            "  --md-inverse-primary:            {{colors.inverse_primary.hex}};\n\n"
            "  /* Secondary */\n"
            "  --md-secondary:                  {{colors.secondary.hex}};\n"
            "  --md-on-secondary:               {{colors.on_secondary.hex}};\n"
            "  --md-secondary-container:        {{colors.secondary_container.hex}};\n"
            "  --md-on-secondary-container:     {{colors.on_secondary_container.hex}};\n\n"
            "  /* Tertiary */\n"
            "  --md-tertiary:                   {{colors.tertiary.hex}};\n"
            "  --md-on-tertiary:                {{colors.on_tertiary.hex}};\n"
            "  --md-tertiary-container:         {{colors.tertiary_container.hex}};\n"
            "  --md-on-tertiary-container:      {{colors.on_tertiary_container.hex}};\n\n"
            "  /* Error */\n"
            "  --md-error:                      {{colors.error.hex}};\n"
            "  --md-on-error:                   {{colors.on_error.hex}};\n"
            "  --md-error-container:            {{colors.error_container.hex}};\n"
            "  --md-on-error-container:         {{colors.on_error_container.hex}};\n\n"
            "  /* Surface */\n"
            "  --md-surface:                    {{colors.surface.hex}};\n"
            "  --md-on-surface:                 {{colors.on_surface.hex}};\n"
            "  --md-surface-variant:            {{colors.surface_variant.hex}};\n"
            "  --md-on-surface-variant:         {{colors.on_surface_variant.hex}};\n"
            "  --md-surface-container-lowest:   {{colors.surface_container_lowest.hex}};\n"
            "  --md-surface-container-low:      {{colors.surface_container_low.hex}};\n"
            "  --md-surface-container:          {{colors.surface_container.hex}};\n"
            "  --md-surface-container-high:     {{colors.surface_container_high.hex}};\n"
            "  --md-surface-container-highest:  {{colors.surface_container_highest.hex}};\n"
            "  --md-surface-bright:             {{colors.surface_bright.hex}};\n"
            "  --md-surface-dim:                {{colors.surface_dim.hex}};\n\n"
            "  /* Background */\n"
            "  --md-background:                 {{colors.background.hex}};\n"
            "  --md-on-background:              {{colors.on_background.hex}};\n\n"
            "  /* Outline */\n"
            "  --md-outline:                    {{colors.outline.hex}};\n"
            "  --md-outline-variant:            {{colors.outline_variant.hex}};\n\n"
            "  /* Inverse / misc */\n"
            "  --md-inverse-surface:            {{colors.inverse_surface.hex}};\n"
            "  --md-inverse-on-surface:         {{colors.inverse_on_surface.hex}};\n"
            "  --md-scrim:                      {{colors.scrim.hex}};\n"
            "  --md-shadow:                     {{colors.shadow.hex}};\n\n"
            "  /* Multi-format examples */\n"
            "  --md-primary-rgb:                {{colors.primary.rgb}};\n"
            "  --md-primary-raw:                {{colors.primary.rgb_raw}};\n"
            "  --md-primary-hsl:                {{colors.primary.hsl}};\n"
            "  --md-primary-light:              {{colors.primary.light.hex}};\n"
            "  --md-primary-dark:               {{colors.primary.dark.hex}};\n"
            "}\n";
        write_file(tplpath,tpl);
        printf("created: %s\n",tplpath);
    }

    printf("\nQuick start:\n");
    printf("  mcugen image ~/Pictures/wallpaper.jpg\n");
    printf("  mcugen color \"#6750A4\" --mode dark\n");
    printf("  mcugen color \"#6750A4\" --type vibrant --mode dark\n");
    printf("  mcugen show color \"#6750A4\" --json\n");
    printf("  mcugen watch ~/Pictures/wallpapers/\n");
}

/* ================================================================
 * Help
 * ================================================================ */
static void print_usage(void) {
    printf(
"mcugen v" MCUGEN_VERSION " — Material Color Utilities Generator\n"
"by Megh Badonia <badoniamegh@gmail.com>\n"
"   github.com/MeghBadonia\n\n"
"Usage:\n"
"  mcugen image <path>             Extract dominant color from image\n"
"  mcugen color <#hex|-|rgb|hsl>   Use a specific color as seed\n"
"  mcugen show  [image|color] ...  Print scheme (no file output)\n"
"  mcugen watch <dir>              Watch directory for new images\n"
"  mcugen reload                   Re-run post-hooks from last run\n"
"  mcugen init                     Scaffold config + example template\n\n"
"Options:\n"
"  -m, --mode <light|dark>         Scheme mode            [default: from config]\n"
"  -t, --type <variant>            Scheme variant         [default: tonal-spot]\n"
"      --contrast <-1.0..1.0>      Contrast level         [default: 0.0]\n"
"      --json                      Output JSON to stdout\n"
"      --dry-run                   Render but do not write files\n"
"      --strict                    Error on unknown template tokens\n"
"  -c, --config <path>             Custom config file\n"
"  -v, --verbose                   Verbose logging\n"
"  -q, --quiet                     Suppress output\n"
"  -h, --help                      Show this help\n"
"      --version                   Print version\n\n"
"Scheme types:\n"
"  tonal-spot (default)  vibrant     expressive  fidelity\n"
"  monochrome            neutral     fruit-salad rainbow   content\n\n"
"Token syntax:  {{colors.<role>.<variant>.<format>}}\n"
"  variant: default | light | dark\n"
"  format:  hex hex_strip HEX HEX_STRIP rgb rgba rgb_raw\n"
"           r g b r_float g_float b_float argb_int argb_hex\n"
"           hsl hct_hue hct_chroma hct_tone\n\n"
"Config input examples for color:\n"
"  mcugen color \"#6750A4\"\n"
"  mcugen color \"rgb(103,80,164)\"\n"
"  echo \"#6750A4\" | mcugen color -\n\n"
"Examples:\n"
"  mcugen image ~/wall.jpg -m dark\n"
"  mcugen image ~/wall.jpg -t vibrant -m dark\n"
"  mcugen color \"#6750A4\" --type expressive --mode light\n"
"  mcugen show color \"#6750A4\" --json | jq .colors.primary\n"
"  mcugen color \"#6750A4\" --dry-run\n"
"  mcugen watch ~/Pictures/walls/\n"
"  mcugen reload\n\n"
"Config: ~/.config/mcugen/config.toml  (or $XDG_CONFIG_HOME/mcugen/)\n"
"Cache:  ~/.cache/mcugen/             (or $XDG_CACHE_HOME/mcugen/)\n"
"Run 'mcugen init' to create a starter config.\n"
    );
}

/* ================================================================
 * main
 * ================================================================ */
int main(int argc, char *argv[]) {
    if(argc<2){print_usage();return 0;}

    int          dark       = -1;
    int          show_only  = 0;
    int          json_out   = 0;
    int          dry_run    = 0;
    SchemeVariant sv        = VARIANT_TONAL_SPOT;
    int          sv_set     = 0;
    char         custom_cfg[MAX_PATH]={0};
    char         seed_str[MAX_PATH]={0};
    int          seed_is_image=0;
    int          do_watch   = 0;
    char         watch_dir[MAX_PATH]={0};

    int argi=1;
    const char *subcmd=argv[argi];

    if(!strcmp(subcmd,"--help")||!strcmp(subcmd,"-h")){print_usage();return 0;}
    if(!strcmp(subcmd,"--version")){printf("mcugen " MCUGEN_VERSION "\n");return 0;}
    if(!strcmp(subcmd,"init")){cmd_init();return 0;}
    if(!strcmp(subcmd,"reload")){cmd_reload();return 0;}

    if(!strcmp(subcmd,"show")){
        show_only=1; argi++;
        if(argi>=argc){print_usage();return 1;}
        subcmd=argv[argi];
    }
    if(!strcmp(subcmd,"watch")){
        do_watch=1; argi++;
        if(argi>=argc) die("'watch' requires a directory path");
        expand_home(argv[argi++],watch_dir,MAX_PATH-1);
    } else if(!strcmp(subcmd,"image")){
        argi++;
        if(argi>=argc) die("'image' requires a path");
        expand_home(argv[argi++],seed_str,MAX_PATH-1);
        seed_is_image=1;
    } else if(!strcmp(subcmd,"color")){
        argi++;
        if(argi>=argc) die("'color' requires a value");
        strncpy(seed_str,argv[argi++],MAX_PATH-1);
        seed_is_image=0;
    } else if(!do_watch){
        if(is_image(subcmd)){
            expand_home(subcmd,seed_str,MAX_PATH-1); seed_is_image=1;
        } else if(subcmd[0]=='#'||(subcmd[0]=='0'&&(subcmd[1]=='x'||subcmd[1]=='X'))){
            strncpy(seed_str,subcmd,MAX_PATH-1); seed_is_image=0;
        } else {
            die("unknown command: '%s'\nRun 'mcugen --help'",subcmd);
        }
        argi++;
    }

    /* Parse remaining flags */
    while(argi<argc){
        const char *a=argv[argi];
        if(!strcmp(a,"-m")||!strcmp(a,"--mode")){
            argi++;
            if(argi>=argc) die("--mode requires light|dark");
            const char *m=argv[argi++];
            if(!strcmp(m,"dark"))       dark=1;
            else if(!strcmp(m,"light")) dark=0;
            else die("--mode: expected light|dark, got '%s'",m);
        } else if(!strcmp(a,"-t")||!strcmp(a,"--type")){
            argi++;
            if(argi>=argc) die("--type requires a variant name");
            sv=parse_variant_type(argv[argi++]); sv_set=1;
        } else if(!strcmp(a,"--contrast")){
            argi++;
            if(argi>=argc) die("--contrast requires a value");
            /* contrast level stored for future use; currently informational */
            (void)strtod(argv[argi++],NULL);
        } else if(!strcmp(a,"--json")){
            json_out=1; argi++;
        } else if(!strcmp(a,"--dry-run")){
            dry_run=1; argi++;
        } else if(!strcmp(a,"--strict")){
            g_strict=1; argi++;
        } else if(!strcmp(a,"-c")||!strcmp(a,"--config")){
            argi++;
            if(argi>=argc) die("--config requires a path");
            expand_home(argv[argi++],custom_cfg,MAX_PATH-1);
        } else if(!strcmp(a,"-v")||!strcmp(a,"--verbose")){
            g_verbose=1; argi++;
        } else if(!strcmp(a,"-q")||!strcmp(a,"--quiet")){
            g_quiet=1; argi++;
        } else {
            die("unknown option: '%s'\nRun 'mcugen --help'",a);
        }
    }

    /* Load config */
    static Config cfg; memset(&cfg,0,sizeof(cfg));
    strcpy(cfg.default_mode,"light");
    strcpy(cfg.default_type,"tonal-spot");

    if(!show_only||json_out||cfg.n_custom_colors||do_watch){
        char cfgpath[MAX_PATH];
        if(custom_cfg[0]) strncpy(cfgpath,custom_cfg,MAX_PATH-1);
        else              config_path(cfgpath,sizeof(cfgpath));
        if(access(cfgpath,F_OK)==0){
            cfg=load_config(cfgpath);
            info("config: %s (%d templates, %d custom colors)",
                 cfgpath,cfg.n_templates,cfg.n_custom_colors);
        } else if(!show_only && !json_out && !do_watch) {
            die("config not found: %s\n  Run 'mcugen init'",cfgpath);
        }
    }

    /* Apply defaults from config */
    if(dark<0)  dark=strcmp(cfg.default_mode,"dark")==0?1:0;
    if(!sv_set) sv=parse_variant_type(cfg.default_type);

    /* Watch mode */
    if(do_watch){
        cmd_watch(watch_dir,dark,sv,&cfg,custom_cfg[0]?custom_cfg:NULL);
        return 0;
    }

    /* Resolve seed */
    int seed_argb=0;
    if(seed_is_image){
        /* Try cache first */
        if(cfg.caching && cache_load_seed(seed_str,&seed_argb)){
            char h[8]; StringUtils_hexFromArgb(seed_argb,h);
            info("cache hit: %s → %s",seed_str,h);
            if(!g_quiet) printf("Seed (cached): %s\n",h);
        } else {
            seed_argb=dominant_from_image(seed_str);
            if(!seed_argb) die("could not extract color from image");
            char h[8]; StringUtils_hexFromArgb(seed_argb,h);
            if(!g_quiet) printf("Seed from image: %s\n",h);
        }
    } else {
        seed_argb=parse_seed_color(seed_str);
        char h[8]; StringUtils_hexFromArgb(seed_argb,h);
        info("seed: %s",h);
    }

    generate(seed_argb,dark,sv,&cfg,show_only,json_out,dry_run,
             seed_is_image?seed_str:NULL);
    return 0;
}
