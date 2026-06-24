#pragma once
#include <stddef.h>

unsigned int crc32_buf(const unsigned char *buf, size_t len);
unsigned int image_hash(const char *path);
void         cache_path(char *out, size_t sz);
int          cache_load_last_seed(int *out_seed);
int          cache_load_seed(const char *image_path, int *out_seed);
void         cache_save(const char *image_path, int seed_argb,
                        const char *mode, const char *type);
