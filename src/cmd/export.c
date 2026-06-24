#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "export.h"
#include "../util/log.h"
#include "../util/fs.h"
#include "../core/scheme.h"
#include "../core/color.h"
#include "../mcugen.h"
#include "mcu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ExportFormat parse_export_format(const char *s) {
    if (!strcmp(s,"figma"))    return EXPORT_FIGMA;
    if (!strcmp(s,"material")) return EXPORT_MATERIAL;
    die("unknown export format '%s' (figma|material)", s);
    return EXPORT_FIGMA;
}

static void write_or_print(const char *path, const char *data) {
    if (path) {
        write_file(path, data);
        printf("Exported: %s\n", path);
    } else {
        fputs(data, stdout);
    }
}

void cmd_export(int seed_argb, int dark, SchemeVariant sv, ExportFormat fmt,
                const char *out_path) {
    Palettes pal = build_palettes(seed_argb, sv);
    FullScheme fs = build_full_scheme(&pal);

    /* build JSON */
    char *buf = malloc(65536);
    if (!buf) die("OOM");
    int pos = 0;

    if (fmt == EXPORT_FIGMA) {
        pos += snprintf(buf+pos, 65536-pos,
            "{\n  \"name\": \"mcugen\",\n  \"variables\": {\n");
        for (int i = 0; g_roles[i].name; i++) {
            RolePair *rp = (RolePair*)((char*)&fs + g_roles[i].offset);
            int argb = dark ? rp->dark : rp->light;
            char hex[8]; StringUtils_hexFromArgb(argb, hex);
            int r=(argb>>16)&0xFF, g2=(argb>>8)&0xFF, b=argb&0xFF;
            pos += snprintf(buf+pos, 65536-pos,
                "    \"%s\": { \"$type\": \"color\", \"$value\": \"#%s\","
                " \"r\": %.4f, \"g\": %.4f, \"b\": %.4f }%s\n",
                g_roles[i].name, hex,
                r/255.0, g2/255.0, b/255.0,
                g_roles[i+1].name ? "," : "");
        }
        pos += snprintf(buf+pos, 65536-pos, "  }\n}\n");
    } else {
        /* Material Design tokens JSON */
        pos += snprintf(buf+pos, 65536-pos,
            "{\n  \"seed\": \"%06X\",\n  \"mode\": \"%s\",\n  \"palettes\": {\n",
            seed_argb & 0xFFFFFF, dark ? "dark" : "light");

        const char *pnames[] = {"primary","secondary","tertiary","neutral","neutral_variant","error",NULL};
        TonalPalette *pals[]  = {&pal.primary,&pal.secondary,&pal.tertiary,
                                  &pal.neutral,&pal.neutral_variant,&pal.error,NULL};
        for (int pi=0; pnames[pi]; pi++) {
            pos += snprintf(buf+pos,65536-pos,"    \"%s\": {", pnames[pi]);
            int tones[]={0,5,10,15,20,25,30,35,40,50,60,70,80,90,95,98,99,100,-1};
            for (int ti=0; tones[ti]>=0; ti++) {
                int argb = TonalPalette_tone(pals[pi], tones[ti]);
                char hex[8]; StringUtils_hexFromArgb(argb, hex);
                pos += snprintf(buf+pos,65536-pos,"%s\"%d\":\"#%s\"",
                    ti?",":"", tones[ti], hex);
            }
            pos += snprintf(buf+pos,65536-pos,"}%s\n", pnames[pi+1]?",":"");
        }
        pos += snprintf(buf+pos,65536-pos,"  },\n  \"roles\": {\n");
        for (int i=0; g_roles[i].name; i++) {
            RolePair *rp = (RolePair*)((char*)&fs + g_roles[i].offset);
            int argb = dark ? rp->dark : rp->light;
            char hex[8]; StringUtils_hexFromArgb(argb, hex);
            pos += snprintf(buf+pos,65536-pos,"    \"%s\": \"#%s\"%s\n",
                g_roles[i].name, hex, g_roles[i+1].name?",":"");
        }
        pos += snprintf(buf+pos,65536-pos,"  }\n}\n");
    }

    write_or_print(out_path, buf);
    free(buf);
}
