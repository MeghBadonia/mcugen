#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "paths.h"
#include "log.h"
#include "../mcugen.h"
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *real_home(void) {
    static char buf[MAX_PATH] = {0};
    if (buf[0]) return buf;
    struct passwd *pw = NULL;
    const char *su = getenv("SUDO_USER");
    if (su && su[0] && strcmp(su,"root")!=0) pw = getpwnam(su);
    if (!pw) {
        const char *u = getenv("USER");
        if (!u||!u[0]) u = getenv("LOGNAME");
        if (u && u[0] && strcmp(u,"root")!=0) pw = getpwnam(u);
    }
    if (!pw) pw = getpwuid(getuid());
    if (pw && pw->pw_dir && pw->pw_dir[0]) {
        snprintf(buf, sizeof(buf), "%s", pw->pw_dir);
        return buf;
    }
    const char *h = getenv("HOME");
    if (h && h[0]) { snprintf(buf, sizeof(buf), "%s", h); return buf; }
    die("cannot determine home directory");
    return NULL;
}

void xdg_config_dir(char *out, size_t outsz) {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0])
        snprintf(out, outsz, "%s/mcugen", xdg);
    else
        snprintf(out, outsz, "%s/.config/mcugen", real_home());
}

void xdg_cache_dir(char *out, size_t outsz) {
    const char *xdg = getenv("XDG_CACHE_HOME");
    if (xdg && xdg[0])
        snprintf(out, outsz, "%s/mcugen", xdg);
    else
        snprintf(out, outsz, "%s/.cache/mcugen", real_home());
}

void config_path(char *out, size_t outsz) {
    char dir[MAX_PATH];
    xdg_config_dir(dir, sizeof(dir));
    snprintf(out, outsz, "%s/%s", dir, CONFIG_FILE);
}

void expand_home(const char *in, char *out, size_t sz) {
    if (in[0]=='~') snprintf(out, sz, "%s%s", real_home(), in+1);
    else            snprintf(out, sz, "%s", in);
}
