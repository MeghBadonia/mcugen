#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "cache.h"
#include "../util/log.h"
#include "../util/paths.h"
#include "../util/fs.h"
#include "../mcugen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

unsigned int crc32_buf(const unsigned char *buf, size_t len) {
    unsigned int crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++)
            crc = (crc>>1) ^ (0xEDB88320 & (-(int)(crc&1)));
    }
    return crc ^ 0xFFFFFFFF;
}

unsigned int image_hash(const char *path) {
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    unsigned char buf[65536]; size_t total = 0;
    unsigned int crc = 0xFFFFFFFF;
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        for (size_t i = 0; i < n; i++) {
            crc ^= buf[i];
            for (int j = 0; j < 8; j++) crc = (crc>>1)^(0xEDB88320&(-(int)(crc&1)));
        }
        total += n;
        if (total > 4*1024*1024) break;
    }
    fclose(f);
    return crc ^ 0xFFFFFFFF;
}

void cache_path(char *out, size_t sz) {
    char dir[MAX_PATH]; xdg_cache_dir(dir, sizeof(dir));
    snprintf(out, sz, "%s/%s", dir, CACHE_FILE);
}

int cache_load_seed(const char *image_path, int *out_seed) {
    char cp[MAX_PATH]; cache_path(cp, sizeof(cp));
    char *src = read_file(cp); if (!src) return 0;
    unsigned int img_hash = image_hash(image_path);
    char hash_str[16]; snprintf(hash_str, sizeof(hash_str), "%08x", img_hash);
    char *ph = strstr(src, "\"hash\":");
    int ok = 0;
    if (ph) {
        ph += 7; while (*ph=='"'||*ph==' ') ph++;
        if (strncmp(ph, hash_str, 8)==0) {
            char *ps = strstr(src, "\"seed\":");
            if (ps) {
                ps += 7; while (*ps=='"'||*ps==' ') ps++;
                *out_seed = (int)strtoul(ps, NULL, 16);
                ok = 1;
            }
        }
    }
    free(src); return ok;
}

void cache_save(const char *image_path, int seed_argb,
                const char *mode, const char *type) {
    char dir[MAX_PATH]; xdg_cache_dir(dir, sizeof(dir));
    mkdirs_local(dir);
    char cp[MAX_PATH]; cache_path(cp, sizeof(cp));
    unsigned int h = image_path ? image_hash(image_path) : 0;
    char json[512];
    snprintf(json, sizeof(json),
        "{\n"
        "  \"hash\": \"%08x\",\n"
        "  \"seed\": \"%08x\",\n"
        "  \"mode\": \"%s\",\n"
        "  \"type\": \"%s\",\n"
        "  \"time\": %ld\n"
        "}\n",
        h, (unsigned)seed_argb, mode, type, (long)time(NULL));
    FILE *f = fopen(cp, "wb");
    if (f) { fputs(json, f); fclose(f); }
}
