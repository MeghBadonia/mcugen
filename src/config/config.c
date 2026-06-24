#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "config.h"
#include "toml.h"
#include "../util/log.h"
#include "../util/paths.h"
#include "../util/fs.h"
#include "../core/token.h"
#include "../mcugen.h"
#include "blend/blend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Config load_config(const char *path) {
    static Config cfg; memset(&cfg, 0, sizeof(cfg));
    strcpy(cfg.default_mode, "light");
    strcpy(cfg.default_type, "tonal-spot");

    char *src = read_file(path);
    if (!src) die("cannot open config: %s\n  Run 'mcugen init' to create one.", path);
    TomlDoc *doc = (TomlDoc*)malloc(sizeof(TomlDoc)); if (!doc) die("oom");
    toml_parse(src, doc); free(src);

    const char *mode = toml_get(doc, "config", "default_mode");
    if (mode) strncpy(cfg.default_mode, mode, 15);
    const char *type = toml_get(doc, "config", "default_type");
    if (type) strncpy(cfg.default_type, type, 31);
    const char *cach = toml_get(doc, "config", "caching");
    if (cach && (!strcmp(cach,"true") || !strcmp(cach,"1"))) cfg.caching = 1;

    char tnames[MAX_TEMPLATES][128];
    int nt = collect_names(doc, "templates.", tnames, MAX_TEMPLATES);
    for (int i = 0; i < nt; i++) {
        char sec[192]; snprintf(sec, sizeof(sec), "templates.%s", tnames[i]);
        const char *inp = toml_get(doc, sec, "input_path");
        if (!inp) { warn("[%s] missing input_path", sec); continue; }

        TemplateConfig *tc = &cfg.templates[cfg.n_templates++];
        strncpy(tc->name, tnames[i], 127);
        expand_home(inp, tc->input_path, MAX_PATH-1);

        char opaths[MAX_OUTPUT_PATHS][MAX_PATH];
        int np  = toml_get_all(doc, sec, "output_path",  opaths,    MAX_OUTPUT_PATHS);
        int np2 = toml_get_all(doc, sec, "output_paths", opaths+np, MAX_OUTPUT_PATHS-np);
        np += np2;
        if (np==0) { warn("[%s] missing output_path", sec); cfg.n_templates--; continue; }
        for (int j = 0; j < np && j < MAX_OUTPUT_PATHS; j++)
            expand_home(opaths[j], tc->output_paths[tc->n_output_paths++], MAX_PATH-1);

        const char *pre  = toml_get(doc, sec, "pre_hook");
        const char *post = toml_get(doc, sec, "post_hook");
        if (pre)  { expand_home(pre,  tc->pre_hook,  MAX_PATH-1); tc->has_pre_hook  = 1; }
        if (post) { expand_home(post, tc->post_hook, MAX_PATH-1); tc->has_post_hook = 1; }
    }

    char cnames[MAX_CUSTOM_COLORS][128];
    int nc = collect_names(doc, "colors.", cnames, MAX_CUSTOM_COLORS);
    for (int i = 0; i < nc; i++) {
        char sec[192]; snprintf(sec, sizeof(sec), "colors.%s", cnames[i]);
        const char *cv = toml_get(doc, sec, "color");
        if (!cv) { warn("[%s] missing color", sec); continue; }
        CustomColorDef *cd = &cfg.custom_colors[cfg.n_custom_colors++];
        strncpy(cd->name, cnames[i], 63);
        strncpy(cd->color_str, cv, 63);
        const char *bl = toml_get(doc, sec, "blend");
        cd->blend = (bl && (!strcmp(bl,"true") || !strcmp(bl,"1"))) ? 1 : 0;
    }

    free(doc);
    return cfg;
}

int resolve_custom_colors(const Config *cfg, int seed_argb, CustomColor *out) {
    int n = 0;
    for (int i = 0; i < cfg->n_custom_colors; i++) {
        const CustomColorDef *cd = &cfg->custom_colors[i];
        const char *p = cd->color_str;
        if (*p=='#') p++;
        else if (p[0]=='0' && (p[1]=='x'||p[1]=='X')) p += 2;
        unsigned long v = strtoul(p, NULL, 16);
        if (strlen(p)==6) v |= 0xFF000000UL;
        int argb = (int)(unsigned int)v;
        if (cd->blend) argb = Blend_harmonize(argb, seed_argb);
        strncpy(out[n].name, cd->name, 63);
        out[n].argb = argb;
        n++;
    }
    return n;
}
