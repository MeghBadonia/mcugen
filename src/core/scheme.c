#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "scheme.h"
#include "../util/log.h"
#include "mcu.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

double g_contrast = 0.0;

SchemeVariant parse_variant_type(const char *s) {
    if (!s || !strcmp(s,"tonal-spot") || !strcmp(s,"tonal_spot")) return VARIANT_TONAL_SPOT;
    if (!strcmp(s,"vibrant"))                     return VARIANT_VIBRANT;
    if (!strcmp(s,"expressive"))                  return VARIANT_EXPRESSIVE;
    if (!strcmp(s,"fidelity"))                    return VARIANT_FIDELITY;
    if (!strcmp(s,"monochrome"))                  return VARIANT_MONOCHROME;
    if (!strcmp(s,"neutral"))                     return VARIANT_NEUTRAL;
    if (!strcmp(s,"fruit-salad") || !strcmp(s,"fruit_salad")) return VARIANT_FRUIT_SALAD;
    if (!strcmp(s,"rainbow"))                     return VARIANT_RAINBOW;
    if (!strcmp(s,"content"))                     return VARIANT_CONTENT;
    warn("unknown scheme type '%s'; using tonal-spot", s);
    return VARIANT_TONAL_SPOT;
}

const char *variant_name(SchemeVariant v) {
    switch (v) {
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

double rotated_hue(double hue, const double hues[], const double rotations[], int n) {
    if (n == 0) return hue;
    if (hue < hues[0]) return MathUtils_sanitizeDegreesDouble(hue + rotations[0]);
    for (int i = 0; i < n-1; i++) {
        if (hue >= hues[i] && hue < hues[i+1]) {
            double t = (hue - hues[i]) / (hues[i+1] - hues[i]);
            double r = rotations[i] + t * (rotations[i+1] - rotations[i]);
            return MathUtils_sanitizeDegreesDouble(hue + r);
        }
    }
    return MathUtils_sanitizeDegreesDouble(hue + rotations[n-1]);
}

Palettes build_palettes(int seed_argb, SchemeVariant variant) {
    Hct src = Hct_fromInt(seed_argb);
    Palettes p;
    double h = src.hue, c = src.chroma;

    switch (variant) {
    case VARIANT_TONAL_SPOT:
        p.primary         = TonalPalette_fromHueAndChroma(h, 36.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h, 16.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h+60.0), 24.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  6.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  8.0);
        break;
    case VARIANT_VIBRANT:
        p.primary = TonalPalette_fromHueAndChroma(h, 200.0);
        { static const double vh[]={0,41,61,101,131,181,251,301,360};
          static const double vr[]={18,15,10,12,15,18,15,12,12};
          p.secondary = TonalPalette_fromHueAndChroma(rotated_hue(h,vh,vr,9), 24.0); }
        { static const double th[]={0,41,61,101,131,181,251,301,360};
          static const double tr[]={35,30,20,25,30,35,30,25,25};
          p.tertiary = TonalPalette_fromHueAndChroma(rotated_hue(h,th,tr,9), 32.0); }
        p.neutral         = TonalPalette_fromHueAndChroma(h, 10.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h, 12.0);
        break;
    case VARIANT_EXPRESSIVE:
        p.primary = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h+240.0), 40.0);
        { static const double sh[]={0,21,51,121,151,191,271,321,360};
          static const double sr[]={45,95,45,20,45,90,45,45,45};
          p.secondary = TonalPalette_fromHueAndChroma(rotated_hue(h,sh,sr,9), 24.0); }
        { static const double th[]={0,21,51,121,151,191,271,321,360};
          static const double tr[]={120,120,20,45,20,15,20,120,120};
          p.tertiary = TonalPalette_fromHueAndChroma(rotated_hue(h,th,tr,9), 32.0); }
        p.neutral         = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h+15.0),  8.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h+15.0), 12.0);
        break;
    case VARIANT_FIDELITY:
    case VARIANT_CONTENT:
        p.primary   = TonalPalette_fromHueAndChroma(h, c);
        p.secondary = TonalPalette_fromHueAndChroma(h, fmax(c-32.0, c*0.5));
        { Hct src_hct = Hct_fromInt(seed_argb);
          TemperatureCache *tc = TemperatureCache_create(src_hct);
          Hct comp = TemperatureCache_complement(tc);
          Hct fixed = DislikeAnalyzer_fixIfDisliked(&comp);
          p.tertiary = TonalPalette_fromHct(&fixed);
          TemperatureCache_free(tc); }
        p.neutral         = TonalPalette_fromHueAndChroma(h, c/8.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h, c/8.0+4.0);
        break;
    case VARIANT_MONOCHROME:
        p.primary = p.secondary = p.tertiary =
        p.neutral = p.neutral_variant = TonalPalette_fromHueAndChroma(h, 0.0);
        break;
    case VARIANT_NEUTRAL:
        p.primary         = TonalPalette_fromHueAndChroma(h, 12.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h,  8.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(h, 16.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  2.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  2.0);
        break;
    case VARIANT_FRUIT_SALAD:
        p.primary         = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h-50.0), 48.0);
        p.secondary       = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h-50.0), 36.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(h, 36.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h, 10.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h, 16.0);
        break;
    case VARIANT_RAINBOW:
        p.primary         = TonalPalette_fromHueAndChroma(h, 48.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h, 16.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h+60.0), 24.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  0.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  0.0);
        break;
    default:
        p.primary         = TonalPalette_fromHueAndChroma(h, 36.0);
        p.secondary       = TonalPalette_fromHueAndChroma(h, 16.0);
        p.tertiary        = TonalPalette_fromHueAndChroma(MathUtils_sanitizeDegreesDouble(h+60.0), 24.0);
        p.neutral         = TonalPalette_fromHueAndChroma(h,  6.0);
        p.neutral_variant = TonalPalette_fromHueAndChroma(h,  8.0);
        break;
    }
    p.error = TonalPalette_fromHueAndChroma(25.0, 84.0);
    return p;
}

/* Apply contrast adjustment to a tone pair.
   "on" colors (text/icon) are pushed further from backgrounds.
   positive g_contrast → more contrast, negative → less. */
static int adjust_tone_light(int base, int is_on) {
    if (g_contrast == 0.0) return base;
    double shift = g_contrast * (is_on ? -10.0 : 5.0);
    double t = (double)base + shift; /* extracted tone proxy (0..100 scale via argb) */
    (void)t;
    return base; /* tone adjustment applied in build_full_scheme via palette tone override */
}

FullScheme build_full_scheme(Palettes *p) {
    FullScheme s;
    double ct = g_contrast; /* -1..1 */

    /* Helper: clamp tone */
#define CLAMP(x) ((x)<0?0:((x)>100?100:(x)))

    /* tone_l / tone_d compute the final tone with contrast adjustment.
       "on" roles get pushed further (more contrast), others stay or get closer. */
#define MK(f, pal, lt, dt) do { \
    double tl = (double)(lt) + ct * -8.0; \
    double td = (double)(dt) + ct *  8.0; \
    s.f.light = TonalPalette_tone(&p->pal, (int)CLAMP(tl)); \
    s.f.dark  = TonalPalette_tone(&p->pal, (int)CLAMP(td)); \
} while(0)

#define MKN(f, pal, lt, dt) do { \
    s.f.light = TonalPalette_tone(&p->pal, lt); \
    s.f.dark  = TonalPalette_tone(&p->pal, dt); \
} while(0)

    /* "on" roles shift more with contrast, content roles shift less */
    MKN(primary,                   primary,          40, 80);
    MK (on_primary,                primary,         100, 20);
    MKN(primary_container,         primary,          90, 30);
    MK (on_primary_container,      primary,          30, 90);
    MKN(inverse_primary,           primary,          80, 40);
    MKN(secondary,                 secondary,        40, 80);
    MK (on_secondary,              secondary,       100, 20);
    MKN(secondary_container,       secondary,        90, 30);
    MK (on_secondary_container,    secondary,        30, 90);
    MKN(tertiary,                  tertiary,         40, 80);
    MK (on_tertiary,               tertiary,        100, 20);
    MKN(tertiary_container,        tertiary,         90, 30);
    MK (on_tertiary_container,     tertiary,         30, 90);
    MKN(error,                     error,            40, 80);
    MK (on_error,                  error,           100, 20);
    MKN(error_container,           error,            90, 30);
    MK (on_error_container,        error,            30, 90);
    MKN(surface,                   neutral,          98,  6);
    MKN(surface_dim,               neutral,          87,  6);
    MKN(surface_bright,            neutral,          98, 24);
    MKN(surface_container_lowest,  neutral,         100,  4);
    MKN(surface_container_low,     neutral,          96, 10);
    MKN(surface_container,         neutral,          94, 12);
    MKN(surface_container_high,    neutral,          92, 17);
    MKN(surface_container_highest, neutral,          90, 22);
    MK (on_surface,                neutral,          10, 90);
    MKN(surface_tint,              primary,          40, 80);
    MKN(surface_variant,           neutral_variant,  90, 30);
    MK (on_surface_variant,        neutral_variant,  30, 80);
    MKN(outline,                   neutral_variant,  50, 60);
    MKN(outline_variant,           neutral_variant,  80, 30);
    MKN(background,                neutral,          98,  6);
    MK (on_background,             neutral,          10, 90);
    MKN(inverse_surface,           neutral,          20, 90);
    MKN(inverse_on_surface,        neutral,          95, 20);
    MKN(scrim,                     neutral,           0,  0);
    MKN(shadow,                    neutral,           0,  0);
    MKN(primary_fixed,             primary,          90, 90);
    MKN(primary_fixed_dim,         primary,          80, 80);
    MKN(on_primary_fixed,          primary,          10, 10);
    MKN(on_primary_fixed_variant,  primary,          30, 30);
    MKN(secondary_fixed,           secondary,        90, 90);
    MKN(secondary_fixed_dim,       secondary,        80, 80);
    MKN(on_secondary_fixed,        secondary,        10, 10);
    MKN(on_secondary_fixed_variant,secondary,        30, 30);
    MKN(tertiary_fixed,            tertiary,         90, 90);
    MKN(tertiary_fixed_dim,        tertiary,         80, 80);
    MKN(on_tertiary_fixed,         tertiary,         10, 10);
    MKN(on_tertiary_fixed_variant, tertiary,         30, 30);
#undef MK
#undef MKN
#undef CLAMP
    return s;
}

#define OFF(f) offsetof(FullScheme,f)
const RoleEntry g_roles[] = {
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
    {NULL, 0}
};
#undef OFF

const RolePair *lookup_role(const FullScheme *s, const char *name) {
    for (int i = 0; g_roles[i].name; i++)
        if (!strcmp(g_roles[i].name, name))
            return (const RolePair*)((const char*)s + g_roles[i].offset);
    return NULL;
}
