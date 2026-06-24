#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int g_verbose = 0;
int g_quiet   = 0;
int g_strict  = 0;

void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;31merror:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void warn(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;33mwarn:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void info(const char *fmt, ...) {
    if (!g_verbose) return;
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;34minfo:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void ok_msg(const char *fmt, ...) {
    if (g_quiet) return;
    va_list ap; va_start(ap, fmt);
    printf("  \033[32m✓\033[0m  ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}
