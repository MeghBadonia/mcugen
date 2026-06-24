#pragma once
#include "color.h"
#include "scheme.h"

typedef struct {
    char name[64];
    int  argb;
} CustomColor;

int   resolve_argb(const RolePair *rp, ColorVariant var, int dark);
int   expand_token(const FullScheme *scheme,
                   const CustomColor *customs, int n_customs,
                   int global_dark,
                   const char *inner,
                   char *buf, size_t sz);
char *render_template(const char *tmpl,
                      const FullScheme *scheme,
                      const CustomColor *customs, int n_customs,
                      int dark);
