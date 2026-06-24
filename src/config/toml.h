#pragma once
#include "../mcugen.h"

#define TOML_MAX_SEC 160
#define TOML_MAX_KEY 128
#define TOML_MAX_VAL MAX_PATH
#define TOML_MAX_ENT 1024

typedef struct {
    char sec[TOML_MAX_SEC];
    char key[TOML_MAX_KEY];
    char val[TOML_MAX_VAL];
} TomlEntry;

typedef struct {
    TomlEntry e[TOML_MAX_ENT];
    int n;
} TomlDoc;

void        toml_trim(char *s);
void        toml_parse(const char *src, TomlDoc *doc);
const char *toml_get(const TomlDoc *d, const char *sec, const char *key);
int         toml_get_all(const TomlDoc *d, const char *sec, const char *key,
                         char out[][MAX_PATH], int max);
int         collect_names(const TomlDoc *d, const char *prefix,
                          char names[][128], int max);
