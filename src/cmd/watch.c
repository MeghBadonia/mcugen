#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#include "watch.h"
#include "generate.h"
#include "../util/log.h"
#include "../core/image.h"
#include "../mcugen.h"
#include "mcu.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

void cmd_watch(const char *watch_dir, int dark, SchemeVariant sv,
               const Config *cfg, const char *custom_cfg_path) {
    (void)custom_cfg_path;
    printf("Watching: %s\n", watch_dir);
    printf("Press Ctrl+C to stop.\n\n");

    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) die("inotify_init1 failed: %s", strerror(errno));

    int wd = inotify_add_watch(fd, watch_dir, IN_CLOSE_WRITE|IN_MOVED_TO|IN_CREATE);
    if (wd < 0) die("cannot watch '%s': %s", watch_dir, strerror(errno));

    char ebuf[4096];
    while (1) {
        ssize_t len = read(fd, ebuf, sizeof(ebuf));
        if (len <= 0) { usleep(500000); continue; }
        int processed = 0;
        char *p = ebuf;
        while (p < ebuf+len) {
            struct inotify_event *ev = (struct inotify_event*)p;
            if (ev->len > 0) {
                char fpath[MAX_PATH];
                snprintf(fpath, sizeof(fpath), "%s/%s", watch_dir, ev->name);
                if (!processed && is_image_ext(ev->name)) {
                    printf("\n\033[1;36m→\033[0m New image: %s\n", fpath);
                    int seed = dominant_from_image(fpath);
                    if (seed) {
                        char h[8]; StringUtils_hexFromArgb(seed, h);
                        printf("  Seed: %s\n", h);
                        generate(seed, dark, sv, cfg, 0, 0, 0, fpath);
                    }
                    processed = 1;
                }
            }
            p += sizeof(struct inotify_event) + ev->len;
        }
    }
    close(fd);
}
