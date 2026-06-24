#pragma once
#include "../mcugen.h"
#include "../core/token.h"

typedef struct {
    char name[128];
    char input_path[MAX_PATH];
    char output_paths[MAX_OUTPUT_PATHS][MAX_PATH];
    int  n_output_paths;
    char pre_hook[MAX_PATH];
    char post_hook[MAX_PATH];
    int  has_pre_hook;
    int  has_post_hook;
} TemplateConfig;

typedef struct {
    char color_str[64];
    int  blend;
    char name[64];
} CustomColorDef;

typedef struct {
    TemplateConfig templates[MAX_TEMPLATES];
    int            n_templates;
    CustomColorDef custom_colors[MAX_CUSTOM_COLORS];
    int            n_custom_colors;
    char           default_mode[16];
    char           default_type[32];
    int            caching;
} Config;

Config load_config(const char *path);
int    resolve_custom_colors(const Config *cfg, int seed_argb, CustomColor *out);
