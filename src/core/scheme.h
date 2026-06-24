#pragma once
#include <stddef.h>
#include "mcu.h"

extern double g_contrast;

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

typedef struct {
    TonalPalette primary, secondary, tertiary;
    TonalPalette neutral, neutral_variant, error;
} Palettes;

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

typedef struct { const char *name; size_t offset; } RoleEntry;
extern const RoleEntry g_roles[];

SchemeVariant    parse_variant_type(const char *s);
const char      *variant_name(SchemeVariant v);
Palettes         build_palettes(int seed_argb, SchemeVariant variant);
FullScheme       build_full_scheme(Palettes *p);
const RolePair  *lookup_role(const FullScheme *s, const char *name);
double           rotated_hue(double hue, const double hues[], const double rotations[], int n);
