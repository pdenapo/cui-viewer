#define _POSIX_C_SOURCE 200809L
#include "thumbnails.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#else
#include <strings.h>
#endif
#include <sys/types.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(d) _mkdir(d)
#else
#include <unistd.h>
#define MKDIR(d) mkdir((d), 0755)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../vendor/CUI/third_party/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../vendor/CUI/third_party/stb_image_write.h"

#ifdef HAVE_MUPDF
#include <mupdf/fitz.h>
#endif

static void mkdir_p(const char *path) {
    char tmp[4096];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char c = *p;
            *p = '\0';
            MKDIR(tmp);
            *p = c;
        }
    }
    MKDIR(tmp);
}

static void path_dirname(const char *src, char *out, size_t cap) {
    snprintf(out, cap, "%s", src ? src : "");
    char *slash = strrchr(out, '/');
    if (!slash) slash = strrchr(out, '\\');
    if (slash) *slash = '\0';
    else snprintf(out, cap, ".");
}

static const char *path_basename(const char *src) {
    const char *p = src;
    const char *last = src;
    while (*p) {
        if (*p == '/' || *p == '\\') last = p + 1;
        p++;
    }
    return last;
}

static void replace_ext_png(const char *name, char *out, size_t cap) {
    snprintf(out, cap, "%s", name);
    char *dot = strrchr(out, '.');
    if (dot) snprintf(dot, cap - (size_t)(dot - out), ".png");
    else {
        size_t n = strlen(out);
        if (n + 5 < cap) snprintf(out + n, cap - n, ".png");
    }
}

bool thumbnail_path_for(const char *source, const char *location, char *out, size_t cap) {
    if (!source || !source[0] || !location || !out || cap == 0) return false;
    if (strcmp(location, "none") == 0) return false;

    if (strcmp(location, "per-directory") == 0) {
        char dir[4096];
        path_dirname(source, dir, sizeof dir);
        char name[4096];
        replace_ext_png(path_basename(source), name, sizeof name);
        snprintf(out, cap, "%s/.xvpics/%s", dir, name);
        return true;
    }

    /* global */
    const char *base = getenv("XDG_CACHE_HOME");
    char dirbuf[4096];
    if (!base || !base[0]) {
        const char *home = getenv("HOME");
        if (!home) home = ".";
        snprintf(dirbuf, sizeof dirbuf, "%s/.cache", home);
        base = dirbuf;
    }
    unsigned long hash = 5381;
    const char *p = source;
    while (*p) { hash = hash * 33 + (unsigned char)*p; p++; }
    char name[64];
    snprintf(name, sizeof name, "%016lx.png", hash);
    snprintf(out, cap, "%s/cuiviewer/thumbnails/%s", base, name);
    /* base points to either env or dirbuf; ensure dirbuf lifetime ok */
    if (base == dirbuf) { /* already copied into out via snprintf? out has path, fine */ }
    return true;
}

bool thumbnail_ensure(const char *source, int size, const char *location, char *out, size_t cap) {
    if (size < 16) size = 128;
    if (!thumbnail_path_for(source, location, out, cap)) return false;

    struct stat src_st;
    if (stat(source, &src_st) != 0) return false;

    struct stat th_st;
    if (stat(out, &th_st) == 0) {
        if (th_st.st_mtime >= src_st.st_mtime) return true;
    }

    int w = 0, h = 0, c = 0;
    unsigned char *img = NULL;
#ifdef HAVE_MUPDF
    {
        size_t slen = strlen(source);
        if (slen >= 4 && strcasecmp(source + slen - 4, ".pdf") == 0) {
            fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
            if (ctx) {
                fz_register_document_handlers(ctx);
                fz_document *doc = NULL;
                fz_try(ctx) { doc = fz_open_document(ctx, source); } fz_catch(ctx) { doc = NULL; }
                if (doc && fz_count_pages(ctx, doc) > 0) {
                    fz_page *pg = NULL;
                    fz_try(ctx) { pg = fz_load_page(ctx, doc, 0); } fz_catch(ctx) { pg = NULL; }
                    if (pg) {
                        float scale = 2.0f;
                        fz_matrix m = fz_scale(scale, scale);
                        fz_pixmap *pix = NULL;
                        fz_try(ctx) { pix = fz_new_pixmap_from_page(ctx, pg, m, fz_device_rgb(ctx), 0); } fz_catch(ctx) { pix = NULL; }
                        fz_drop_page(ctx, pg);
                        if (pix) {
                            w = fz_pixmap_width(ctx, pix);
                            h = fz_pixmap_height(ctx, pix);
                            int comps = fz_pixmap_components(ctx, pix);
                            unsigned char *samples = fz_pixmap_samples(ctx, pix);
                            int stride = fz_pixmap_stride(ctx, pix);
                            img = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
                            if (img) {
                                for (int y = 0; y < h; y++) {
                                    for (int x = 0; x < w; x++) {
                                        unsigned char *s = samples + (size_t)y * (size_t)stride + (size_t)x * (size_t)comps;
                                        unsigned char *d = img + ((size_t)y * (size_t)w + (size_t)x) * 3;
                                        d[0] = comps >= 1 ? s[0] : 0;
                                        d[1] = comps >= 2 ? s[1] : d[0];
                                        d[2] = comps >= 3 ? s[2] : d[0];
                                    }
                                }
                            }
                            fz_drop_pixmap(ctx, pix);
                        }
                    }
                }
                if (doc) fz_drop_document(ctx, doc);
            }
            fz_drop_context(ctx);
        }
    }
#endif
    if (!img) {
        img = stbi_load(source, &w, &h, &c, 3);
    }
    if (!img || w <= 0 || h <= 0) {
        if (img) {
            /* could be stbi or malloc; free appropriately */
            stbi_image_free(img);
        }
        return false;
    }

    int tw, th;
    if (w >= h) {
        tw = size;
        th = (int)((double)h / (double)w * (double)size);
        if (th < 1) th = 1;
    } else {
        th = size;
        tw = (int)((double)w / (double)h * (double)size);
        if (tw < 1) tw = 1;
    }

    unsigned char *thumb = (unsigned char *)malloc((size_t)tw * (size_t)th * 3);
    if (!thumb) {
        stbi_image_free(img);
        return false;
    }
    for (int y = 0; y < th; y++) {
        for (int x = 0; x < tw; x++) {
            int sx = x * w / tw;
            int sy = y * h / th;
            if (sx >= w) sx = w - 1;
            if (sy >= h) sy = h - 1;
            for (int k = 0; k < 3; k++) {
                thumb[(y * tw + x) * 3 + k] = img[(sy * w + sx) * 3 + k];
            }
        }
    }

    /* ensure parent dir */
    char dir[4096];
    path_dirname(out, dir, sizeof dir);
    mkdir_p(dir);

    int ok = stbi_write_png(out, tw, th, 3, thumb, tw * 3);
    free(thumb);
    stbi_image_free(img);
    return ok != 0;
}

void thumbnail_cache_init(void) {
    /* no-op for now */
}