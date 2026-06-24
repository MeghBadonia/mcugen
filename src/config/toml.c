#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "toml.h"
#include "../util/log.h"
#include "../mcugen.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void toml_trim(char *s) {
    char *p = s; while (*p && isspace((unsigned char)*p)) p++;
    memmove(s, p, strlen(p)+1);
    int l = (int)strlen(s);
    while (l > 0 && isspace((unsigned char)s[l-1])) s[--l] = '\0';
}

void toml_parse(const char *src, TomlDoc *doc) {
    doc->n = 0;
    char sec[TOML_MAX_SEC] = "";
    char line[MAX_LINE];
    const char *p = src;
    while (*p) {
        int i = 0;
        while (*p && *p!='\n' && i<MAX_LINE-1) line[i++] = *p++;
        if (*p=='\n') p++;
        line[i] = '\0'; toml_trim(line);
        if (!line[0] || line[0]=='#') continue;
        if (line[0]=='[') {
            char *end = strchr(line, ']'); if (!end) continue;
            *end = '\0'; strncpy(sec, line+1, TOML_MAX_SEC-1); toml_trim(sec);
            continue;
        }
        char *eq = strchr(line, '='); if (!eq) continue;
        *eq = '\0';
        char key[TOML_MAX_KEY], val[TOML_MAX_VAL];
        strncpy(key, line, TOML_MAX_KEY-1); toml_trim(key);
        strncpy(val, eq+1, TOML_MAX_VAL-1); toml_trim(val);
        {
            int in_q = 0;
            for (int ci = 0; val[ci]; ci++) {
                if (val[ci]=='"') in_q = !in_q;
                if (!in_q && val[ci]==' ' && val[ci+1]=='#') { val[ci]='\0'; break; }
            }
        }
        toml_trim(val);
        size_t vl = strlen(val);
        if (vl>=2 && ((val[0]=='"'&&val[vl-1]=='"')||(val[0]=='\''&&val[vl-1]=='\'')))
            { val[vl-1]='\0'; memmove(val, val+1, vl-1); }
        if (doc->n >= TOML_MAX_ENT) { warn("TOML limit reached"); break; }
        snprintf(doc->e[doc->n].sec, TOML_MAX_SEC, "%s", sec);
        snprintf(doc->e[doc->n].key, TOML_MAX_KEY, "%s", key);
        snprintf(doc->e[doc->n].val, TOML_MAX_VAL, "%s", val);
        doc->n++;
    }
}

const char *toml_get(const TomlDoc *d, const char *sec, const char *key) {
    for (int i = 0; i < d->n; i++)
        if (!strcmp(d->e[i].sec, sec) && !strcmp(d->e[i].key, key))
            return d->e[i].val;
    return NULL;
}

int toml_get_all(const TomlDoc *d, const char *sec, const char *key,
                 char out[][MAX_PATH], int max) {
    int n = 0;
    for (int i = 0; i < d->n && n < max; i++)
        if (!strcmp(d->e[i].sec, sec) && !strcmp(d->e[i].key, key))
            snprintf(out[n++], MAX_PATH, "%s", d->e[i].val);
    return n;
}

int collect_names(const TomlDoc *d, const char *prefix,
                  char names[][128], int max) {
    int n = 0;
    size_t pl = strlen(prefix);
    for (int i = 0; i < d->n; i++) {
        if (strncmp(d->e[i].sec, prefix, pl) != 0) continue;
        const char *nm = d->e[i].sec + pl;
        int dup = 0;
        for (int j = 0; j < n; j++) if (!strcmp(names[j], nm)) { dup=1; break; }
        if (!dup && n < max) strncpy(names[n++], nm, 127);
    }
    return n;
}
