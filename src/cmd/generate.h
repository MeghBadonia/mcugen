#pragma once
#include "../config/config.h"
#include "../core/scheme.h"

void generate(int seed_argb, int dark, SchemeVariant sv,
              const Config *cfg,
              int show_only, int json_out, int dry_run,
              const char *image_path);

void show_scheme(int seed_argb, int dark, SchemeVariant sv,
                 const FullScheme *scheme,
                 const CustomColor *customs, int n_customs);

void print_json(int seed_argb, int dark,
                const FullScheme *s,
                const CustomColor *customs, int n_customs,
                const char *type_str);
