#pragma once
#include <stddef.h>

const char *real_home(void);
void xdg_config_dir(char *out, size_t outsz);
void xdg_cache_dir(char *out, size_t outsz);
void config_path(char *out, size_t outsz);
void expand_home(const char *in, char *out, size_t sz);
