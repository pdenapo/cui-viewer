#define _POSIX_C_SOURCE 200809L
#include "pdf_viewer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <string.h>
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#else
#include <strings.h>
#endif

#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_MUPDF
#include <mupdf/fitz.h>
#endif

/* stb not needed here; MuPDF writes PNG directly. */

#include <cui/cui.h>

struct pdf_viewer {
    char path[4096];
#ifdef HAVE_MUPDF
    void *ctx;
    void *doc;
#endif
    cui_texture texture;
    int image_w;
    int image_h;
    cui_image_view view;
    int page;
    int page_count;
    bool is_open;
};

pdf_viewer *pdf_viewer_create(void) {
    pdf_viewer *v = (pdf_viewer *)calloc(1, sizeof(*v));
    if (v) {
        v->page = 0;
        v->page_count = 0;
        v->texture = 0;
        v->is_open = false;
    }
    return v;
}

static void close_impl(pdf_viewer *v, cui_ctx *ui) {
    if (!v) return;
    if (v->texture) {
        cui_image_destroy(ui, v->texture);
        v->texture = 0;
    }
#ifdef HAVE_MUPDF
    if (v->doc) {
        fz_drop_document((fz_context *)v->ctx, (fz_document *)v->doc);
        v->doc = NULL;
    }
    if (v->ctx) {
        fz_drop_context((fz_context *)v->ctx);
        v->ctx = NULL;
    }
#endif
    v->is_open = false;
    v->page = 0;
    v->page_count = 0;
    v->path[0] = '\0';
}

void pdf_viewer_destroy(pdf_viewer *v, void *ui) {
    if (!v) return;
    close_impl(v, (cui_ctx *)ui);
    free(v);
}

bool pdf_viewer_open(pdf_viewer *v, void *ui, const char *path) {
    if (!v || !path || !path[0]) return false;
    cui_ctx *cui = (cui_ctx *)ui;
    /* close previous */
    if (v->texture) { cui_image_destroy(cui, v->texture); v->texture = 0; }
#ifdef HAVE_MUPDF
    if (v->doc) { fz_drop_document((fz_context *)v->ctx, (fz_document *)v->doc); v->doc = NULL; }
    if (v->ctx) { fz_drop_context((fz_context *)v->ctx); v->ctx = NULL; }
#endif
    v->is_open = false;
    v->page = 0;
    v->page_count = 0;

    snprintf(v->path, sizeof v->path, "%s", path);

#ifdef HAVE_MUPDF
    fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
    if (!ctx) return false;
    fz_register_document_handlers(ctx);
    fz_document *doc = NULL;
    fz_try(ctx) { doc = fz_open_document(ctx, path); } fz_catch(ctx) { doc = NULL; }
    if (!doc) { fz_drop_context(ctx); return false; }
    v->ctx = ctx;
    v->doc = doc;
    int n = 0;
    fz_try(ctx) { n = fz_count_pages(ctx, doc); } fz_catch(ctx) { n = 0; }
    if (n <= 0) { close_impl(v, cui); return false; }
    v->page_count = n;
    v->is_open = true;
    /* render page 0 */
    if (!pdf_viewer_goto(v, cui, 0)) { close_impl(v, cui); return false; }
    return true;
#else
    (void)cui;
    return false;
#endif
}

void pdf_viewer_close(pdf_viewer *v, void *ui) {
    close_impl(v, (cui_ctx *)ui);
}

static bool render_page(pdf_viewer *v, cui_ctx *ui, int page) {
#ifdef HAVE_MUPDF
    if (!v || !v->doc) return false;
    fz_context *ctx = (fz_context *)v->ctx;
    fz_document *doc = (fz_document *)v->doc;
    int n = fz_count_pages(ctx, doc);
    if (page < 0 || page >= n) return false;
    fz_page *pg = NULL;
    fz_pixmap *pix = NULL;
    fz_try(ctx) { pg = fz_load_page(ctx, doc, page); } fz_catch(ctx) { pg = NULL; }
    if (!pg) return false;
    fz_matrix m = fz_scale(2.0f, 2.0f);
    fz_try(ctx) { pix = fz_new_pixmap_from_page(ctx, pg, m, fz_device_rgb(ctx), 0); } fz_catch(ctx) { pix = NULL; }
    fz_drop_page(ctx, pg);
    if (!pix) return false;
    int w = fz_pixmap_width(ctx, pix);
    int h = fz_pixmap_height(ctx, pix);
    int comps = fz_pixmap_components(ctx, pix);
    unsigned char *samples = fz_pixmap_samples(ctx, pix);
    int stride = fz_pixmap_stride(ctx, pix);
    unsigned char *rgba = (unsigned char *)malloc((size_t)w * (size_t)h * 4);
    if (!rgba) { fz_drop_pixmap(ctx, pix); return false; }
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned char *s = samples + (size_t)y * (size_t)stride + (size_t)x * (size_t)comps;
            unsigned char *d = rgba + ((size_t)y * (size_t)w + (size_t)x) * 4;
            d[0] = comps >= 1 ? s[0] : 0;
            d[1] = comps >= 2 ? s[1] : d[0];
            d[2] = comps >= 3 ? s[2] : d[0];
            d[3] = 255;
        }
    }
    if (v->texture) { cui_image_destroy(ui, v->texture); v->texture = 0; }
    cui_texture t = cui_image_create(ui, w, h, rgba);
    free(rgba);
    fz_drop_pixmap(ctx, pix);
    if (!t) return false;
    v->texture = t;
    v->image_w = w;
    v->image_h = h;
    v->page = page;
    v->page_count = n;
    v->view.zoom = 0.0f;
    v->view.cx = (float)w * 0.5f;
    v->view.cy = (float)h * 0.5f;
    return true;
#else
    (void)v; (void)ui; (void)page;
    return false;
#endif
}

bool pdf_viewer_goto(pdf_viewer *v, void *ui, int page) {
    if (!v || !v->is_open) return false;
    return render_page(v, (cui_ctx *)ui, page);
}

bool pdf_viewer_first(pdf_viewer *v, void *ui) { return pdf_viewer_goto(v, ui, 0); }
bool pdf_viewer_last(pdf_viewer *v, void *ui) {
    if (!v) return false;
    return pdf_viewer_goto(v, ui, v->page_count - 1);
}
bool pdf_viewer_prev(pdf_viewer *v, void *ui) {
    if (!v) return false;
    return pdf_viewer_goto(v, ui, v->page - 1);
}
bool pdf_viewer_next(pdf_viewer *v, void *ui) {
    if (!v) return false;
    return pdf_viewer_goto(v, ui, v->page + 1);
}

bool pdf_viewer_is_open(const pdf_viewer *v) { return v ? v->is_open : false; }
const char *pdf_viewer_path(const pdf_viewer *v) { return v ? v->path : ""; }
int pdf_viewer_page(const pdf_viewer *v) { return v ? v->page : 0; }
int pdf_viewer_page_count(const pdf_viewer *v) { return v ? v->page_count : 0; }
void *pdf_viewer_texture(const pdf_viewer *v) { return v ? (void *)v->texture : NULL; }
int pdf_viewer_width(const pdf_viewer *v) { return v ? v->image_w : 0; }
int pdf_viewer_height(const pdf_viewer *v) { return v ? v->image_h : 0; }
void *pdf_viewer_view(pdf_viewer *v) { return v ? (void *)&v->view : NULL; }

bool pdf_viewer_print_range(pdf_viewer *v, void *ui, int first_page, int last_page) {
#ifdef HAVE_MUPDF
    if (!v || !v->is_open) return false;
    if (first_page < 1) first_page = 1;
    if (last_page > v->page_count) last_page = v->page_count;
    if (first_page > last_page) return false;

    /* Render pages to temp PNGs. */
    char names[64][256];
    int nfiles = 0;
    for (int p = first_page; p <= last_page; p++) {
        fz_context *ctx = (fz_context *)v->ctx;
        fz_document *doc = (fz_document *)v->doc;
        fz_page *pg = NULL;
        fz_pixmap *pix = NULL;
        fz_try(ctx) { pg = fz_load_page(ctx, doc, p - 1); } fz_catch(ctx) { pg = NULL; }
        if (!pg) continue;
        fz_matrix m = fz_scale(2.0f, 2.0f);
        fz_try(ctx) { pix = fz_new_pixmap_from_page(ctx, pg, m, fz_device_rgb(ctx), 0); } fz_catch(ctx) { pix = NULL; }
        fz_drop_page(ctx, pg);
        if (!pix) continue;
#ifdef _WIN32
        char tmpname[256];
        snprintf(tmpname, sizeof tmpname, "%s\\cuiviewer_print_%d.png", getenv("TEMP") ? getenv("TEMP") : ".", p);
#else
        char tmpname[256];
        snprintf(tmpname, sizeof tmpname, "/tmp/cuiviewer_print_%d.png", p);
#endif
        fz_save_pixmap_as_png(ctx, pix, tmpname);
        fz_drop_pixmap(ctx, pix);
        if (nfiles < 64) {
            snprintf(names[nfiles], sizeof names[nfiles], "%s", tmpname);
            nfiles++;
        }
    }
    if (nfiles == 0) return false;

    int rc = 0;
#ifdef _WIN32
    /* Use PowerShell's Start-Process -Verb Print for each file. */
    for (int i = 0; i < nfiles; i++) {
        char cmd[1024];
        /* escape single quotes for PowerShell */
        char esc[256];
        size_t ei = 0;
        for (size_t ci = 0; names[i][ci] && ei + 2 < sizeof esc; ci++) {
            if (names[i][ci] == '\'') { esc[ei++] = '\''; esc[ei++] = '\''; }
            else esc[ei++] = names[i][ci];
        }
        esc[ei] = '\0';
        snprintf(cmd, sizeof cmd, "powershell -NoProfile -Command \"Start-Process -FilePath '%s' -Verb Print\" >NUL 2>&1", esc);
        if (system(cmd) != 0) rc = 1;
    }
#else
    /* Try CUPS lp, then lpr. */
    char cmd[8192] = "";
    size_t ci = 0;
    const char *prefix = "lp ";
    size_t plen = strlen(prefix);
    memcpy(cmd, prefix, plen); ci = plen;
    for (int i = 0; i < nfiles && ci < sizeof cmd - 256; i++) {
        size_t tl = strlen(names[i]);
        if (ci + tl + 2 >= sizeof cmd) break;
        memcpy(cmd + ci, names[i], tl); ci += tl;
        cmd[ci++] = ' ';
    }
    cmd[ci] = '\0';
    /* append redirect */
    char full[8192];
    snprintf(full, sizeof full, "%s >/dev/null 2>&1", cmd);
    rc = system(full);
    if (rc != 0) {
        ci = 0;
        const char *prefix2 = "lpr ";
        plen = strlen(prefix2);
        memcpy(cmd, prefix2, plen); ci = plen;
        for (int i = 0; i < nfiles && ci < sizeof cmd - 256; i++) {
            size_t tl = strlen(names[i]);
            if (ci + tl + 2 >= sizeof cmd) break;
            memcpy(cmd + ci, names[i], tl); ci += tl;
            cmd[ci++] = ' ';
        }
        cmd[ci] = '\0';
        snprintf(full, sizeof full, "%s >/dev/null 2>&1", cmd);
        rc = system(full);
    }
#endif

    /* cleanup */
    for (int i = 0; i < nfiles; i++) {
        remove(names[i]);
    }
    return rc == 0;
#else
    (void)v; (void)ui; (void)first_page; (void)last_page;
    return false;
#endif
}