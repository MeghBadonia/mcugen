#pragma once
#include <stdarg.h>

extern int g_verbose;
extern int g_quiet;
extern int g_strict;

void die(const char *fmt, ...);
void warn(const char *fmt, ...);
void info(const char *fmt, ...);
void ok_msg(const char *fmt, ...);
