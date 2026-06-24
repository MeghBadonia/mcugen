#pragma once
#include "../config/config.h"

void cmd_save_profile(const char *name, const char *seed_hex, int dark, const char *scheme_type);
void cmd_use_profile(const char *name, const Config *cfg);
void cmd_list_profiles(void);
