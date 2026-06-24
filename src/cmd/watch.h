#pragma once
#include "../config/config.h"
#include "../core/scheme.h"

void cmd_watch(const char *watch_dir, int dark, SchemeVariant sv,
               const Config *cfg, const char *custom_cfg_path);
