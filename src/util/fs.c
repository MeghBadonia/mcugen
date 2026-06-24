#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "fs.h"
#include "log.h"
#include "paths.h"
#include "../mcugen.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    if (sz < 0) { fclose(f); return NULL; }
    char *buf = (char*)malloc((size_t)sz+1); if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz] = '\0'; fclose(f); return buf;
}

int path_is_in_home(const char *path) {
    const char *h = real_home(); size_t hl = strlen(h);
    return strncmp(path, h, hl)==0 && (path[hl]=='/' || path[hl]=='\0');
}

void mkdirs_local(const char *dir_path) {
    char dir[MAX_PATH]; snprintf(dir, sizeof(dir), "%s", dir_path);
    for (char *p = dir+1; *p; p++) {
        if (*p=='/') { *p='\0'; mkdir(dir, 0755); *p='/'; }
    }
    mkdir(dir, 0755);
}

int mkdirs_sudo(const char *dir_path) {
    char cmd[MAX_PATH+32];
    snprintf(cmd, sizeof(cmd), "sudo mkdir -p '%s'", dir_path);
    return system(cmd);
}

void sudo_keepalive(void) {
    system("sudo -v 2>/dev/null");
}

int write_file(const char *path, const char *buf) {
    char dir[MAX_PATH]; snprintf(dir, sizeof(dir), "%s", path);
    char *sl = strrchr(dir, '/'); if (sl) *sl='\0'; else dir[0]='\0';

    if (path_is_in_home(path)) {
        if (dir[0]) mkdirs_local(dir);
        FILE *f = fopen(path, "wb");
        if (!f) {
            warn("cannot write '%s': %s", path, strerror(errno));
            warn("hint: run  sudo chown -R $USER '%s'", dir[0]?dir:path);
            return -1;
        }
        fputs(buf, f); fclose(f);
        const char *su = getenv("SUDO_USER");
        if (su && su[0] && getuid()==0) {
            char cmd[MAX_PATH+64];
            snprintf(cmd, sizeof(cmd), "chown '%s' '%s' 2>/dev/null", su, path);
            (void)system(cmd);
            if (dir[0]) {
                snprintf(cmd, sizeof(cmd), "chown '%s' '%s' 2>/dev/null", su, dir);
                (void)system(cmd);
            }
        }
        return 0;
    } else {
        if (dir[0] && mkdirs_sudo(dir)!=0) {
            warn("sudo mkdir failed for '%s'", dir); return -1;
        }
        char tmp[MAX_PATH];
        snprintf(tmp, sizeof(tmp), "/tmp/mcugen_%d", (int)getpid());
        FILE *tf = fopen(tmp, "wb"); if (!tf) { warn("temp file failed"); return -1; }
        fputs(buf, tf); fclose(tf);
        char cmd[MAX_PATH*2+32];
        snprintf(cmd, sizeof(cmd), "sudo tee '%s' < '%s' > /dev/null", path, tmp);
        int r = system(cmd); remove(tmp);
        if (r!=0) { warn("sudo tee failed for '%s'", path); return -1; }
        return 0;
    }
}
