#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "check.h"
#include "../util/log.h"
#include "../util/fs.h"
#include "../core/scheme.h"
#include "../mcugen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

static void check_ok(const char *msg) {
    printf("  \033[32m✓\033[0m  %s\n", msg);
}
static void check_fail(const char *msg) {
    printf("  \033[31m✗\033[0m  %s\n", msg);
}
static void check_warn(const char *msg) {
    printf("  \033[33m~\033[0m  %s\n", msg);
}

static void scan_tokens(const char *tmpl_src, const char *tpl_name) {
    const char *p = tmpl_src;
    int unknown = 0;
    while (*p) {
        if (p[0]=='{' && p[1]=='{') {
            const char *end = strstr(p+2, "}}");
            if (!end) { p++; continue; }
            size_t ilen = (size_t)(end - (p+2));
            char inner[256] = {0};
            if (ilen >= sizeof(inner)) ilen = sizeof(inner)-1;
            memcpy(inner, p+2, ilen);
            /* trim whitespace */
            char *s = inner;
            while (*s==' '||*s=='\t') s++;
            size_t sl = strlen(s);
            while (sl>0 && (s[sl-1]==' '||s[sl-1]=='\t')) s[--sl]='\0';

            if (strncmp(s, "colors.", 7)==0) {
                char role[64] = {0};
                sscanf(s+7, "%63[^.]", role);
                if (!lookup_role(NULL, role)) {
                    /* check if it's a known role by trying lookup with a dummy scheme */
                    /* just check against g_roles table */
                    int found = 0;
                    for (int i = 0; g_roles[i].name; i++)
                        if (!strcmp(g_roles[i].name, role)) { found=1; break; }
                    if (!found) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "  unknown token role '%s' in [%s]", role, tpl_name);
                        check_warn(msg);
                        unknown++;
                    }
                }
            }
            p = end + 2;
        } else {
            p++;
        }
    }
    if (unknown == 0) {
        char msg[128]; snprintf(msg, sizeof(msg), "tokens ok in [%s]", tpl_name);
        check_ok(msg);
    }
}

void cmd_check(const Config *cfg, const char *cfg_path) {
    int errors = 0;

    /* Config file */
    {
        char msg[MAX_PATH+32];
        snprintf(msg, sizeof(msg), "config: %s", cfg_path);
        check_ok(msg);
    }

    if (cfg->n_templates == 0) {
        check_warn("no templates defined in config");
    }

    for (int i = 0; i < cfg->n_templates; i++) {
        const TemplateConfig *tc = &cfg->templates[i];
        char msg[MAX_PATH+64];

        /* input_path */
        if (access(tc->input_path, R_OK) == 0) {
            snprintf(msg, sizeof(msg), "template [%s] input: %s", tc->name, tc->input_path);
            check_ok(msg);

            /* scan tokens */
            char *src = read_file(tc->input_path);
            if (src) {
                scan_tokens(src, tc->name);
                free(src);
            }
        } else {
            snprintf(msg, sizeof(msg), "template [%s] input missing: %s", tc->name, tc->input_path);
            check_fail(msg);
            errors++;
        }

        /* output paths — check parent dir */
        for (int j = 0; j < tc->n_output_paths; j++) {
            const char *op = tc->output_paths[j];
            char dir[MAX_PATH]; snprintf(dir, sizeof(dir), "%s", op);
            char *sl = strrchr(dir, '/');
            if (sl) {
                *sl = '\0';
                struct stat st;
                if (stat(dir, &st)==0 && S_ISDIR(st.st_mode)) {
                    snprintf(msg, sizeof(msg), "template [%s] output dir ok: %s", tc->name, dir);
                    check_ok(msg);
                } else {
                    snprintf(msg, sizeof(msg), "template [%s] output dir missing: %s (will be created)", tc->name, dir);
                    check_warn(msg);
                }
            }
        }
    }

    printf("\n");
    if (errors == 0)
        printf("\033[32mAll checks passed.\033[0m\n");
    else
        printf("\033[31m%d error(s) found.\033[0m\n", errors);
}
