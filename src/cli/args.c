#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "args.h"
#include "help.h"
#include "../util/log.h"
#include "../util/paths.h"
#include "../config/config.h"
#include "../core/scheme.h"
#include "../core/image.h"
#include "../cmd/generate.h"
#include "../cmd/reload.h"
#include "../cmd/watch.h"
#include "../cmd/init.h"
#include "../cmd/check.h"
#include "../cmd/profiles.h"
#include "../cmd/blend_cmd.h"
#include "../cmd/export.h"
#include "../cache/cache.h"
#include "../mcugen.h"
#include "mcu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Returns ARGB int on success, 0 on invalid input. */
static int parse_hex_color(const char *s) {
    if (!s) return 0;
    const char *p = (s[0] == '#') ? s + 1 : s;
    int len = (int)strlen(p);
    if (len != 6 && len != 3) return 0;
    for (int i = 0; i < len; i++) {
        char c = p[i];
        if (!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) return 0;
    }
    unsigned long v = strtoul(p, NULL, 16);
    if (len == 3) {
        unsigned r = (v>>8)&0xF, g = (v>>4)&0xF, b = v&0xF;
        v = (r<<20)|(r<<16)|(g<<12)|(g<<8)|(b<<4)|b;
    }
    return (int)(0xFF000000u | (unsigned)v);
}

static SchemeVariant parse_sv(const char *s) {
    if (!strcmp(s,"tonal-spot"))   return VARIANT_TONAL_SPOT;
    if (!strcmp(s,"vibrant"))      return VARIANT_VIBRANT;
    if (!strcmp(s,"expressive"))   return VARIANT_EXPRESSIVE;
    if (!strcmp(s,"fidelity"))     return VARIANT_FIDELITY;
    if (!strcmp(s,"monochrome"))   return VARIANT_MONOCHROME;
    if (!strcmp(s,"neutral"))      return VARIANT_NEUTRAL;
    if (!strcmp(s,"fruit-salad"))  return VARIANT_FRUIT_SALAD;
    if (!strcmp(s,"rainbow"))      return VARIANT_RAINBOW;
    if (!strcmp(s,"content"))      return VARIANT_CONTENT;
    die("unknown scheme type: %s", s);
    return VARIANT_TONAL_SPOT;
}

void parse_args(int argc, char **argv, Args *out) {
    memset(out, 0, sizeof(*out));
    out->sv = VARIANT_TONAL_SPOT;

    if (argc < 2) { print_usage(); exit(0); }

    int i = 1;

    /* subcommand */
    const char *sub = argv[i++];

    if (!strcmp(sub,"--help") || !strcmp(sub,"-h")) { print_usage(); exit(0); }
    if (!strcmp(sub,"--version"))  { printf("mcugen %s\n", MCUGEN_VERSION); exit(0); }

    if      (!strcmp(sub,"color"))    out->cmd = CMD_COLOR;
    else if (!strcmp(sub,"image"))    out->cmd = CMD_IMAGE;
    else if (!strcmp(sub,"blend"))    out->cmd = CMD_BLEND;
    else if (!strcmp(sub,"show"))     out->cmd = CMD_SHOW;
    else if (!strcmp(sub,"save"))     out->cmd = CMD_SAVE;
    else if (!strcmp(sub,"use"))      out->cmd = CMD_USE;
    else if (!strcmp(sub,"profiles")) out->cmd = CMD_PROFILES;
    else if (!strcmp(sub,"check"))    out->cmd = CMD_CHECK;
    else if (!strcmp(sub,"reload"))   out->cmd = CMD_RELOAD;
    else if (!strcmp(sub,"watch"))    out->cmd = CMD_WATCH;
    else if (!strcmp(sub,"init"))     out->cmd = CMD_INIT;
    else { fprintf(stderr,"unknown command: %s\n", sub); print_usage(); exit(1); }

    /* collect positional args before flags */
    for (; i < argc; i++) {
        if (argv[i][0] == '-') break;
        switch (out->cmd) {
        case CMD_COLOR:
            if (!out->seed_hex) { out->seed_hex = argv[i]; }
            break;
        case CMD_IMAGE:
            if (!out->image_path) { out->image_path = argv[i]; }
            break;
        case CMD_BLEND:
            if (!out->seed_hex)  { out->seed_hex  = argv[i]; }
            else if (!out->seed_hex2) { out->seed_hex2 = argv[i]; }
            break;
        case CMD_SHOW:
            if (!out->show_sub) out->show_sub = argv[i];
            else if (!strcmp(out->show_sub,"color") && !out->seed_hex) out->seed_hex = argv[i];
            else if (!strcmp(out->show_sub,"image") && !out->image_path) out->image_path = argv[i];
            break;
        case CMD_SAVE:
        case CMD_USE:
            if (!out->profile_name) out->profile_name = argv[i];
            /* for save, second positional = seed */
            else if (out->cmd==CMD_SAVE && !out->seed_hex) out->seed_hex = argv[i];
            break;
        case CMD_WATCH:
            if (!out->watch_dir) out->watch_dir = argv[i];
            break;
        default: break;
        }
    }

    /* flags */
    for (; i < argc; i++) {
        if (!strcmp(argv[i],"--mode") && i+1<argc) {
            i++; out->dark = !strcmp(argv[i],"dark");
        } else if (!strcmp(argv[i],"--type") && i+1<argc) {
            i++; out->sv = parse_sv(argv[i]);
        } else if (!strcmp(argv[i],"--contrast") && i+1<argc) {
            i++; g_contrast = atof(argv[i]);
        } else if (!strcmp(argv[i],"--json")) {
            out->as_json = 1;
        } else if (!strcmp(argv[i],"--swatch")) {
            out->as_swatch = 1;
        } else if (!strcmp(argv[i],"--config") && i+1<argc) {
            i++; out->config_path = argv[i];
        } else if ((!strcmp(argv[i],"-o") || !strcmp(argv[i],"--output")) && i+1<argc) {
            i++; out->export_out = argv[i];
        } else if (!strcmp(argv[i],"--export") && i+1<argc) {
            i++; out->do_export = 1; out->export_fmt = parse_export_format(argv[i]);
        } else if (!strcmp(argv[i],"-v") || !strcmp(argv[i],"--verbose")) {
            g_verbose = 1;
        } else if (!strcmp(argv[i],"-q") || !strcmp(argv[i],"--quiet")) {
            g_quiet = 1;
        } else if (!strcmp(argv[i],"--strict")) {
            g_strict = 1;
        } else if (!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h")) {
            print_usage(); exit(0);
        } else {
            /* unknown flag: allow positionals that start with # */
            if (argv[i][0]=='#') {
                if (out->cmd==CMD_COLOR && !out->seed_hex) out->seed_hex = argv[i];
                else if (out->cmd==CMD_BLEND && !out->seed_hex) out->seed_hex = argv[i];
                else if (out->cmd==CMD_BLEND && !out->seed_hex2) out->seed_hex2 = argv[i];
            } else {
                fprintf(stderr,"unknown option: %s\n", argv[i]);
            }
        }
    }
}

void dispatch(const Args *a) {
    /* load config */
    char cfg_path[MAX_PATH];
    if (a->config_path) {
        snprintf(cfg_path, sizeof(cfg_path), "%s", a->config_path);
    } else {
        config_path(cfg_path, sizeof(cfg_path));
    }

    /* init and reload don't need a config to exist */
    if (a->cmd == CMD_INIT)  { cmd_init();   return; }
    if (a->cmd == CMD_RELOAD){ cmd_reload(); return; }

    Config cfg = load_config(cfg_path);

    switch (a->cmd) {

    case CMD_CHECK:
        cmd_check(&cfg, cfg_path);
        break;

    case CMD_PROFILES:
        cmd_list_profiles();
        break;

    case CMD_SAVE: {
        if (!a->profile_name) die("save requires a profile name");
        char cached_hex[16] = "";
        const char *sh = a->seed_hex;
        if (!sh) {
            /* load last-used seed from cache */
            int cached_seed = 0;
            if (cache_load_last_seed(&cached_seed) && cached_seed) {
                snprintf(cached_hex, sizeof(cached_hex), "#%06x",
                         (unsigned)(cached_seed & 0xFFFFFF));
                sh = cached_hex;
            }
        }
        if (!sh) die("save: no seed specified and no cached seed found; run 'mcugen color <#hex>' first");
        const char *st = variant_name(a->sv);
        cmd_save_profile(a->profile_name, sh, a->dark, st);
        break;
    }

    case CMD_USE:
        if (!a->profile_name) die("use requires a profile name");
        cmd_use_profile(a->profile_name, &cfg);
        break;

    case CMD_WATCH: {
        const char *wd = a->watch_dir ? a->watch_dir : ".";
        cmd_watch(wd, a->dark, a->sv, &cfg, cfg_path);
        break;
    }

    case CMD_BLEND: {
        if (!a->seed_hex || !a->seed_hex2)
            die("blend requires two colors: mcugen blend <#hex> <#hex>");
        cmd_blend(a->seed_hex, a->seed_hex2, a->dark, a->sv, &cfg,
                  0, a->as_json, a->as_swatch);
        break;
    }

    case CMD_SHOW: {
        int show_only = 1;
        if (!a->show_sub) die("show requires 'color' or 'image'");
        if (!strcmp(a->show_sub,"color")) {
            if (!a->seed_hex) die("show color requires a #hex argument");
            int seed = parse_hex_color(a->seed_hex);
            if (!seed) die("invalid hex color: '%s'", a->seed_hex);
            if (a->do_export)
                cmd_export(seed, a->dark, a->sv, a->export_fmt, a->export_out);
            else
                generate(seed, a->dark, a->sv, &cfg, show_only, a->as_json, a->as_swatch, NULL);
        } else if (!strcmp(a->show_sub,"image")) {
            if (!a->image_path) die("show image requires a path");
            int seed = dominant_from_image(a->image_path);
            if (!seed) die("could not extract color from image");
            if (a->do_export)
                cmd_export(seed, a->dark, a->sv, a->export_fmt, a->export_out);
            else
                generate(seed, a->dark, a->sv, &cfg, show_only, a->as_json, a->as_swatch, a->image_path);
        } else {
            die("show: unknown sub-command '%s'", a->show_sub);
        }
        break;
    }

    case CMD_COLOR: {
        if (!a->seed_hex) die("color requires a #hex argument");
        int seed = parse_hex_color(a->seed_hex);
        if (!seed) die("invalid hex color: '%s'", a->seed_hex);
        if (a->do_export)
            cmd_export(seed, a->dark, a->sv, a->export_fmt, a->export_out);
        else
            generate(seed, a->dark, a->sv, &cfg, 0, a->as_json, a->as_swatch, NULL);
        break;
    }

    case CMD_IMAGE: {
        if (!a->image_path) die("image requires a path");
        int seed = dominant_from_image(a->image_path);
        if (!seed) die("could not extract color from image");
        char hex[8]; StringUtils_hexFromArgb(seed, hex);
        if (!g_quiet) printf("Seed: #%s\n", hex);
        if (a->do_export)
            cmd_export(seed, a->dark, a->sv, a->export_fmt, a->export_out);
        else
            generate(seed, a->dark, a->sv, &cfg, 0, a->as_json, a->as_swatch, a->image_path);
        break;
    }

    default:
        print_usage();
        break;
    }
}
