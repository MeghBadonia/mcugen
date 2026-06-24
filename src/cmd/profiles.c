#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "profiles.h"
#include "generate.h"
#include "../util/log.h"
#include "../util/paths.h"
#include "../util/fs.h"
#include "../core/scheme.h"
#include "../mcugen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

static void profiles_dir(char *buf, size_t n) {
    char base[MAX_PATH]; xdg_config_dir(base, sizeof(base));
    snprintf(buf, n, "%s/profiles", base);
}

void cmd_save_profile(const char *name, const char *seed_hex,
                      int dark, const char *scheme_type) {
    char dir[MAX_PATH]; profiles_dir(dir, sizeof(dir));
    mkdirs_local(dir);

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s.toml", dir, name);

    char content[512];
    snprintf(content, sizeof(content),
        "# mcugen profile: %s\n"
        "seed        = \"%s\"\n"
        "mode        = \"%s\"\n"
        "scheme_type = \"%s\"\n",
        name, seed_hex,
        dark ? "dark" : "light",
        scheme_type ? scheme_type : "tonal-spot");

    write_file(path, content);
    printf("Profile saved: %s\n", path);
}

void cmd_use_profile(const char *name, const Config *cfg) {
    char dir[MAX_PATH]; profiles_dir(dir, sizeof(dir));
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s.toml", dir, name);

    char *src = read_file(path);
    if (!src) die("profile not found: %s", path);

    char seed_hex[32] = {0};
    char mode[16]     = "light";
    char stype[64]    = "tonal-spot";

    char *line = src;
    while (*line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        char key[64]={0}, val[128]={0};
        if (sscanf(line, " %63[^= ] = \"%127[^\"]\"", key, val)==2) {
            if (!strcmp(key,"seed"))        snprintf(seed_hex,sizeof(seed_hex),"%s",val);
            else if (!strcmp(key,"mode"))   snprintf(mode,sizeof(mode),"%s",val);
            else if (!strcmp(key,"scheme_type")) snprintf(stype,sizeof(stype),"%s",val);
        }
        if (!nl) break;
        line = nl+1;
    }
    free(src);

    if (!seed_hex[0]) die("profile %s has no seed", name);

    printf("Using profile: %s  seed=%s  mode=%s  type=%s\n",
           name, seed_hex, mode, stype);

    /* parse seed */
    unsigned long argb = strtoul(seed_hex[0]=='#' ? seed_hex+1 : seed_hex, NULL, 16);
    int seed = (int)(0xFF000000u | (unsigned)argb);
    int dark = !strcmp(mode,"dark");
    generate(seed, dark, 0 /* SV_TONAL_SPOT */, cfg, 0, 0, 0, NULL);
}

void cmd_list_profiles(void) {
    char dir[MAX_PATH]; profiles_dir(dir, sizeof(dir));
    DIR *d = opendir(dir);
    if (!d) {
        printf("No profiles found (dir: %s)\n", dir);
        return;
    }
    printf("Profiles in %s:\n", dir);
    struct dirent *e;
    while ((e = readdir(d))) {
        size_t nl = strlen(e->d_name);
        if (nl > 5 && !strcmp(e->d_name+nl-5, ".toml")) {
            char name[256]; snprintf(name,sizeof(name),"%s",e->d_name);
            name[nl-5]='\0';
            printf("  %s\n", name);
        }
    }
    closedir(d);
}
