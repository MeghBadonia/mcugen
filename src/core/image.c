#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wunused-result"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#include "stb_image.h"

#include "image.h"
#include "../util/log.h"
#include "../mcugen.h"
#include "mcu.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct { float r, g, b; } Vec3f;

static float v3d2(Vec3f a, Vec3f b) {
    float dr=a.r-b.r, dg=a.g-b.g, db=a.b-b.b;
    return dr*dr + dg*dg + db*db;
}

static float v3score(Vec3f c, long cnt) {
    float mx = c.r>c.g?(c.r>c.b?c.r:c.b):(c.g>c.b?c.g:c.b);
    float mn = c.r<c.g?(c.r<c.b?c.r:c.b):(c.g<c.b?c.g:c.b);
    float ch = mx - mn;
    float lu = 0.299f*c.r + 0.587f*c.g + 0.114f*c.b;
    return (float)cnt * ch * (1.0f - fabsf(lu - 0.45f));
}

static int median_cut(Vec3f *px, long n, Vec3f *out, int k) {
    typedef struct { float sr,sg,sb; long cnt; } Bucket;
    static Bucket buckets[16*16*16];
    memset(buckets, 0, sizeof(buckets));
    for (long i = 0; i < n; i++) {
        int ri = (int)(px[i].r*15.0f+0.5f);
        int gi = (int)(px[i].g*15.0f+0.5f);
        int bi = (int)(px[i].b*15.0f+0.5f);
        int idx = ri*256 + gi*16 + bi;
        buckets[idx].sr += px[i].r;
        buckets[idx].sg += px[i].g;
        buckets[idx].sb += px[i].b;
        buckets[idx].cnt++;
    }
    static int order[16*16*16];
    for (int i = 0; i < 16*16*16; i++) order[i] = i;
    int nout = 0;
    for (int i = 0; i < 16*16*16 && nout < k; i++) {
        int mi = i;
        for (int j = i+1; j < 16*16*16; j++)
            if (buckets[order[j]].cnt > buckets[order[mi]].cnt) mi = j;
        int tmp = order[i]; order[i] = order[mi]; order[mi] = tmp;
        if (buckets[order[i]].cnt > 0) {
            long c = buckets[order[i]].cnt;
            out[nout].r = buckets[order[i]].sr / c;
            out[nout].g = buckets[order[i]].sg / c;
            out[nout].b = buckets[order[i]].sb / c;
            nout++;
        }
    }
    return nout;
}

int dominant_from_image(const char *path) {
    int w, h, ch;
    unsigned char *data = stbi_load(path, &w, &h, &ch, 3);
    if (!data) die("cannot load image '%s': %s", path, stbi_failure_reason());

    long total = (long)w*h;
    long stride = total/MAX_PIXELS; if (stride<1) stride=1;
    long ns = total/stride; if (ns<1) ns=1;

    Vec3f *px = (Vec3f*)malloc(ns*sizeof(Vec3f));
    if (!px) die("out of memory");
    for (long i = 0; i < ns; i++) {
        long idx = i*stride;
        px[i].r = data[idx*3+0]/255.0f;
        px[i].g = data[idx*3+1]/255.0f;
        px[i].b = data[idx*3+2]/255.0f;
    }
    stbi_image_free(data);

    Vec3f mc_centers[MEDCUT_BINS];
    int mc_n = median_cut(px, ns, mc_centers, MEDCUT_BINS);

    int k = (KMEANS_K < mc_n) ? KMEANS_K : mc_n;
    Vec3f cen[KMEANS_K]; long cnt[KMEANS_K]; Vec3f sum[KMEANS_K];
    int asgn[MEDCUT_BINS] = {0};
    for (int i = 0; i < k; i++) cen[i] = mc_centers[i*mc_n/k];

    for (int it = 0; it < KMEANS_ITERS; it++) {
        int changed = 0;
        for (int i = 0; i < mc_n; i++) {
            float bd=1e30f; int bk=0;
            for (int kk=0; kk<k; kk++) { float d=v3d2(mc_centers[i],cen[kk]); if(d<bd){bd=d;bk=kk;} }
            if (asgn[i]!=bk) { asgn[i]=bk; changed++; }
        }
        if (it > 0 && changed==0) break;
        memset(cnt, 0, sizeof(cnt)); memset(sum, 0, sizeof(sum));
        for (int i = 0; i < mc_n; i++) {
            int kk = asgn[i]; cnt[kk]++;
            sum[kk].r += mc_centers[i].r;
            sum[kk].g += mc_centers[i].g;
            sum[kk].b += mc_centers[i].b;
        }
        for (int kk = 0; kk < k; kk++) if (cnt[kk]>0) {
            cen[kk].r = sum[kk].r/cnt[kk];
            cen[kk].g = sum[kk].g/cnt[kk];
            cen[kk].b = sum[kk].b/cnt[kk];
        }
    }
    free(px);

    float bs = -1; int bk = 0;
    for (int kk = 0; kk < k; kk++) {
        float s = v3score(cen[kk], cnt[kk]);
        if (s > bs) { bs=s; bk=kk; }
    }

    int r  = (int)(cen[bk].r*255.0f+0.5f); if (r>255)  r=255;
    int g2 = (int)(cen[bk].g*255.0f+0.5f); if (g2>255) g2=255;
    int b2 = (int)(cen[bk].b*255.0f+0.5f); if (b2>255) b2=255;
    return (int)(0xFF000000u | ((unsigned)r<<16) | ((unsigned)g2<<8) | (unsigned)b2);
}

int is_image_ext(const char *name) {
    if (!name) return 0;
    const char *e = strrchr(name, '.');
    if (!e) return 0;
    char lc[8] = {0}; strncpy(lc, e, 7);
    for (int i = 0; lc[i]; i++) lc[i] = (char)tolower((unsigned char)lc[i]);
    return !strcmp(lc,".jpg")||!strcmp(lc,".jpeg")||
           !strcmp(lc,".png")||!strcmp(lc,".bmp")||!strcmp(lc,".gif");
}

int is_image(const char *s) {
    const char *e = strrchr(s, '.');
    if (!e) return 0;
    return is_image_ext(e);
}
