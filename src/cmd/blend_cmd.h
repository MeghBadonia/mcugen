#pragma once
#include "../config/config.h"
#include "../core/scheme.h"

void cmd_blend(const char *hex1, const char *hex2, int dark, SchemeVariant sv,
               const Config *cfg, int show_only, int as_json, int as_swatch);
