#pragma once
#include "../core/scheme.h"

typedef enum { EXPORT_FIGMA, EXPORT_MATERIAL } ExportFormat;

void cmd_export(int seed_argb, int dark, SchemeVariant sv, ExportFormat fmt,
                const char *out_path);
ExportFormat parse_export_format(const char *s);
