#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "token.h"
#include "../util/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int resolve_argb(const RolePair *rp, ColorVariant var, int dark) {
    if (var == VAR_LIGHT) return rp->light;
    if (var == VAR_DARK)  return rp->dark;
    return dark ? rp->dark : rp->light;
}

int expand_token(const FullScheme *scheme,
                 const CustomColor *customs, int n_customs,
                 int global_dark,
                 const char *inner,
                 char *buf, size_t sz) {
    if (strncmp(inner, "colors.", 7) != 0) return 0;
    const char *rest = inner + 7;

    char role_s[64]={0}, p2[32]={0}, p3[32]={0};
    int n = sscanf(rest, "%63[^.].%31[^.].%31s", role_s, p2, p3);
    if (n < 1) return 0;

    const RolePair *rp = lookup_role(scheme, role_s);
    if (rp) {
        ColorVariant var = VAR_DEFAULT;
        ColorFormat  fmt = FMT_HEX;
        if (n==2) {
            ColorVariant tv = parse_variant(p2);
            ColorFormat  tf = parse_format(p2);
            if (!strcmp(p2,"default")||!strcmp(p2,"light")||!strcmp(p2,"dark"))
                var = tv;
            else if (tf != FMT_UNKNOWN) fmt = tf;
            else warn("unrecognised token part '%s'", p2);
        } else if (n==3) {
            var = parse_variant(p2);
            fmt = parse_format(p3);
            if (fmt == FMT_UNKNOWN) { warn("unknown format '%s'", p3); fmt = FMT_HEX; }
        }
        int argb = resolve_argb(rp, var, global_dark);
        format_color(argb, fmt, buf, sz);
        return 1;
    }

    for (int i = 0; i < n_customs; i++) {
        if (!strcmp(customs[i].name, role_s)) {
            ColorFormat fmt = FMT_HEX;
            if (n >= 2) {
                ColorFormat tf = parse_format(p2);
                if (tf != FMT_UNKNOWN) fmt = tf;
                else if (n >= 3) { tf = parse_format(p3); if (tf != FMT_UNKNOWN) fmt = tf; }
            }
            format_color(customs[i].argb, fmt, buf, sz);
            return 1;
        }
    }

    if (g_strict) warn("unknown color role: '%s'", role_s);
    return 0;
}

char *render_template(const char *tmpl,
                      const FullScheme *scheme,
                      const CustomColor *customs, int n_customs,
                      int dark) {
    size_t tlen = strlen(tmpl);
    size_t bufsz = tlen*4 + 4096;
    char *out = (char*)malloc(bufsz);
    if (!out) die("out of memory");
    size_t op = 0, ip = 0;

    while (ip < tlen) {
        if (tmpl[ip]=='{' && tmpl[ip+1]=='{') {
            const char *end = strstr(tmpl+ip+2, "}}");
            if (!end) { out[op++] = tmpl[ip++]; continue; }
            size_t ilen = (size_t)(end - (tmpl+ip+2));
            char inner[256] = {0};
            if (ilen >= sizeof(inner)) ilen = sizeof(inner)-1;
            const char *is = tmpl+ip+2;
            while (*is==' '||*is=='\t') { is++; ilen--; }
            size_t ie = ilen;
            while (ie > 0 && (is[ie-1]==' '||is[ie-1]=='\t')) ie--;
            memcpy(inner, is, ie); inner[ie] = '\0';

            char val[128] = {0};
            if (expand_token(scheme, customs, n_customs, dark, inner, val, sizeof(val))) {
                size_t vl = strlen(val);
                if (op+vl+1 >= bufsz) { bufsz=(op+vl+1)*2+4096; out=(char*)realloc(out,bufsz); }
                memcpy(out+op, val, vl); op += vl;
            } else {
                size_t rl = (size_t)(end+2 - (tmpl+ip));
                if (op+rl+1 >= bufsz) { bufsz=(op+rl+1)*2; out=(char*)realloc(out,bufsz); }
                memcpy(out+op, tmpl+ip, rl); op += rl;
            }
            ip = (size_t)(end-tmpl) + 2;
        } else {
            out[op++] = tmpl[ip++];
            if (op >= bufsz-2) { bufsz *= 2; out = (char*)realloc(out, bufsz); }
        }
    }
    out[op] = '\0';
    return out;
}
