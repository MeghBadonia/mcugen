#pragma once
#include "../core/scheme.h"
#include "../cmd/export.h"

typedef struct {
    /* subcommand */
    enum {
        CMD_NONE=0, CMD_COLOR, CMD_IMAGE,
        CMD_BLEND, CMD_SHOW,
        CMD_SAVE, CMD_USE, CMD_PROFILES,
        CMD_CHECK, CMD_RELOAD, CMD_WATCH, CMD_INIT,
    } cmd;

    /* positional arguments */
    const char *seed_hex;    /* color/blend/save/use */
    const char *seed_hex2;   /* blend second color   */
    const char *image_path;  /* image                */
    const char *watch_dir;   /* watch                */
    const char *profile_name;/* save/use             */
    const char *show_sub;    /* show color|image     */

    /* options */
    int         dark;
    SchemeVariant sv;  /* uses VARIANT_* enum values */
    double      contrast;
    int         as_json;
    int         as_swatch;
    int         show_only;
    const char *config_path;
    const char *export_out;
    int         do_export;
    ExportFormat export_fmt;
} Args;

void parse_args(int argc, char **argv, Args *out);
void dispatch(const Args *a);
