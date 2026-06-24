#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "generate.h"
#include "reload.h"
#include "../util/log.h"
#include "../util/fs.h"
#include "../util/paths.h"
#include "../core/token.h"
#include "../core/scheme.h"
#include "../cache/cache.h"
#include "../config/config.h"
#include "../mcugen.h"
#include "mcu.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const TemplateConfig *tc;
    const FullScheme     *scheme;
    const CustomColor    *customs;
    int                   n_customs;
    int                   dark;
    int                   dry_run;
    int                   success;
    int                   skipped;
} RenderJob;

static void *render_job(void *arg) {
    RenderJob *job = (RenderJob*)arg;
    const TemplateConfig *tc = job->tc;
    job->success = 0; job->skipped = 0;

    if (tc->has_pre_hook) {
        info("pre_hook [%s]: %s", tc->name, tc->pre_hook);
        int ret = system(tc->pre_hook);
        if (ret != 0) {
            warn("[%s] pre_hook failed (exit %d) — skipping template", tc->name, ret);
            job->skipped = 1;
            return NULL;
        }
    }

    char *tmpl_src = read_file(tc->input_path);
    if (!tmpl_src) {
        warn("[%s] cannot read template '%s'", tc->name, tc->input_path);
        return NULL;
    }
    char *rendered = render_template(tmpl_src, job->scheme,
                                     job->customs, job->n_customs, job->dark);
    free(tmpl_src);

    int all_ok = 1;
    for (int j = 0; j < tc->n_output_paths; j++) {
        const char *op = tc->output_paths[j];
        if (job->dry_run) {
            if (!g_quiet)
                printf("  \033[36m~\033[0m  [%s]  %s  \033[2m(dry-run)\033[0m\n",
                       tc->name, op);
        } else {
            if (write_file(op, rendered)==0) {
                if (!g_quiet) {
                    int ih = path_is_in_home(op);
                    printf("  \033[32m✓\033[0m  [%s]  %s%s\n",
                           tc->name, op, ih ? "" : " \033[33m(sudo)\033[0m");
                }
            } else {
                all_ok = 0;
            }
        }
    }
    free(rendered);

    if (all_ok) job->success = 1;

    if (tc->has_post_hook && (all_ok || job->dry_run)) {
        info("post_hook [%s]: %s", tc->name, tc->post_hook);
        int ret = system(tc->post_hook);
        if (ret != 0) warn("[%s] post_hook exited %d", tc->name, ret);
    }
    return NULL;
}

void print_json(int seed_argb, int dark,
                const FullScheme *s,
                const CustomColor *customs, int n_customs,
                const char *type_str) {
    char h[8]; StringUtils_hexFromArgb(seed_argb, h);
    Hct src = Hct_fromInt(seed_argb);
    printf("{\n");
    printf("  \"seed\": \"%s\",\n", h);
    printf("  \"mode\": \"%s\",\n", dark?"dark":"light");
    printf("  \"type\": \"%s\",\n", type_str);
    printf("  \"hct\": { \"hue\": %.4f, \"chroma\": %.4f, \"tone\": %.4f },\n",
           src.hue, src.chroma, src.tone);
    printf("  \"colors\": {\n");
    for (int i = 0; g_roles[i].name; i++) {
        const RolePair *rp = (const RolePair*)((const char*)s + g_roles[i].offset);
        char lh[8], dh[8];
        StringUtils_hexFromArgb(rp->light, lh);
        StringUtils_hexFromArgb(rp->dark,  dh);
        printf("    \"%s\": { \"light\": \"%s\", \"dark\": \"%s\" }",
               g_roles[i].name, lh, dh);
        int has_next = g_roles[i+1].name != NULL || n_customs > 0;
        printf("%s\n", has_next ? "," : "");
    }
    for (int i = 0; i < n_customs; i++) {
        char ch2[8]; StringUtils_hexFromArgb(customs[i].argb, ch2);
        printf("    \"%s\": \"%s\"%s\n", customs[i].name, ch2,
               i < n_customs-1 ? "," : "");
    }
    printf("  }\n}\n");
}

/* Print scheme table with ANSI true-color swatches */
void show_scheme(int seed_argb, int dark, SchemeVariant sv,
                 const FullScheme *scheme,
                 const CustomColor *customs, int n_customs) {
    char h[8]; StringUtils_hexFromArgb(seed_argb, h);
    Hct src = Hct_fromInt(seed_argb);
    printf("Seed: %s  (H=%.1f C=%.1f T=%.1f)  mode=%s  type=%s\n\n",
           h, src.hue, src.chroma, src.tone,
           dark?"dark":"light", variant_name(sv));
    printf("%-40s  %-14s  %s\n", "Role", "Light", "Dark");
    printf("%-40s  %-14s  %s\n",
           "--------------------------------------", "-----------", "-----------");
    for (int i = 0; g_roles[i].name; i++) {
        const RolePair *rp = (const RolePair*)((const char*)scheme + g_roles[i].offset);
        char lh[8], dh[8];
        StringUtils_hexFromArgb(rp->light, lh);
        StringUtils_hexFromArgb(rp->dark,  dh);
        int lr = (rp->light >> 16) & 0xff;
        int lg = (rp->light >>  8) & 0xff;
        int lb =  rp->light        & 0xff;
        int dr = (rp->dark  >> 16) & 0xff;
        int dg = (rp->dark  >>  8) & 0xff;
        int db =  rp->dark         & 0xff;
        printf("%-40s  \033[48;2;%d;%d;%dm  \033[0m %s  \033[48;2;%d;%d;%dm  \033[0m %s\n",
               g_roles[i].name,
               lr, lg, lb, lh,
               dr, dg, db, dh);
    }
    if (n_customs > 0) {
        printf("\nCustom colors:\n");
        for (int i = 0; i < n_customs; i++) {
            char h2[8]; StringUtils_hexFromArgb(customs[i].argb, h2);
            int r2 = (customs[i].argb >> 16) & 0xff;
            int g2 = (customs[i].argb >>  8) & 0xff;
            int b2 =  customs[i].argb        & 0xff;
            printf("  %-30s  \033[48;2;%d;%d;%dm  \033[0m %s\n",
                   customs[i].name, r2, g2, b2, h2);
        }
    }
}

void generate(int seed_argb, int dark, SchemeVariant sv,
              const Config *cfg,
              int show_only, int json_out, int dry_run,
              const char *image_path) {
    Palettes pal = build_palettes(seed_argb, sv);
    static FullScheme scheme;
    scheme = build_full_scheme(&pal);

    CustomColor customs[MAX_CUSTOM_COLORS];
    int n_customs = resolve_custom_colors(cfg, seed_argb, customs);

    if (json_out) {
        print_json(seed_argb, dark, &scheme, customs, n_customs, variant_name(sv));
        return;
    }
    if (show_only) {
        show_scheme(seed_argb, dark, sv, &scheme, customs, n_customs);
        return;
    }

    for (int i = 0; i < cfg->n_templates; i++)
        for (int j = 0; j < cfg->templates[i].n_output_paths; j++)
            if (!path_is_in_home(cfg->templates[i].output_paths[j])) {
                sudo_keepalive(); goto done_sudo;
            }
    done_sudo:;

    RenderJob jobs[MAX_TEMPLATES];
    pthread_t threads[MAX_TEMPLATES];
    for (int i = 0; i < cfg->n_templates; i++) {
        jobs[i].tc        = &cfg->templates[i];
        jobs[i].scheme    = &scheme;
        jobs[i].customs   = customs;
        jobs[i].n_customs = n_customs;
        jobs[i].dark      = dark;
        jobs[i].dry_run   = dry_run;
        jobs[i].success   = 0;
        jobs[i].skipped   = 0;
        pthread_create(&threads[i], NULL, render_job, &jobs[i]);
    }

    int ok = 0, skipped = 0;
    for (int i = 0; i < cfg->n_templates; i++) {
        pthread_join(threads[i], NULL);
        if (jobs[i].skipped) skipped++;
        else if (jobs[i].success) ok++;
    }

    save_hooks_cache(cfg);

    if (image_path && cfg->caching)
        cache_save(image_path, seed_argb, dark?"dark":"light", variant_name(sv));
    else if (!image_path)
        cache_save(NULL, seed_argb, dark?"dark":"light", variant_name(sv));

    if (!g_quiet) {
        char h[8]; StringUtils_hexFromArgb(seed_argb, h);
        printf("\n\033[1mDone.\033[0m  Seed=%s  mode=%s  type=%s  "
               "ok=%d  skipped=%d  failed=%d\n",
               h, dark?"dark":"light", variant_name(sv),
               ok, skipped, cfg->n_templates-ok-skipped);
        if (dry_run) printf("  \033[2m(dry-run — no files written)\033[0m\n");
    }
}
