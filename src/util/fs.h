#pragma once
#include <stddef.h>

char *read_file(const char *path);
int   write_file(const char *path, const char *buf);
void  mkdirs_local(const char *dir_path);
int   mkdirs_sudo(const char *dir_path);
int   path_is_in_home(const char *path);
void  sudo_keepalive(void);
