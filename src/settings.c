#define _POSIX_C_SOURCE 200809L
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#define MKDIR(d) _mkdir(d)
#else
#define MKDIR(d) mkdir((d), 0755)
#endif

#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
#include <windows.h>
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

/* Minimal JSON helpers. */

static const char *find_key(const char *json, const char *key) {
    char needle[256];
    snprintf(needle, sizeof needle, "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return NULL;
    p += strlen(needle);
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != ':') return NULL;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

static int parse_string(const char *json, const char *key, char *out, size_t cap) {
    const char *p = find_key(json, key);
    if (!p || *p != '"') return 0;
    p++;
    size_t n = 0;
    while (*p && *p != '"' && n + 1 < cap) {
        if (*p == '\\' && p[1]) {
            p++;
        }
        out[n++] = *p++;
    }
    out[n] = '\0';
    return 1;
}

static int parse_int(const char *json, const char *key, int *out) {
    const char *p = find_key(json, key);
    if (!p) return 0;
    char *end = NULL;
    long v = strtol(p, &end, 10);
    if (end == p) return 0;
    *out = (int)v;
    return 1;
}

static int parse_double(const char *json, const char *key, double *out) {
    const char *p = find_key(json, key);
    if (!p) return 0;
    char *end = NULL;
    double v = strtod(p, &end);
    if (end == p) return 0;
    *out = v;
    return 1;
}

static int parse_bool(const char *json, const char *key, bool *out) {
    const char *p = find_key(json, key);
    if (!p) return 0;
    if (strncmp(p, "true", 4) == 0) { *out = true; return 1; }
    if (strncmp(p, "false", 5) == 0) { *out = false; return 1; }
    return 0;
}

void settings_defaults(app_settings *s) {
    if (!s) return;
    memset(s, 0, sizeof *s);
    s->lang = 0; /* LANG_EN */
    s->dark = false;
    s->last_directory[0] = '\0';
    s->window_width = 1280;
    s->window_height = 800;
    s->window_maximized = true;
    s->viewer_checkerboard = true;
    s->viewer_max_zoom = 16.0f;
    s->slideshow_shuffle = false;
    s->slideshow_recursive = false;
    s->slideshow_wrap = true;
    s->slideshow_interval_seconds = 5.0;
    s->thumbnails_enabled = true;
    snprintf(s->thumbnails_location, sizeof s->thumbnails_location, "per-directory");
    s->thumbnails_size = 128;
    s->thumbnails_quality = 75;
    s->thumbnails_show_directories = false;
    s->thumbnails_slow_update = false;
}

int settings_load(app_settings *s, const char *path) {
    if (!s) return -1;
    settings_defaults(s);
    if (!path || !path[0]) return 0;

    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 1024 * 1024) {
        fclose(f);
        return 0;
    }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return -1; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';

    int v;
    if (parse_int(buf, "language", &v)) { /* accept numeric too, but prefer code below */ }
    char code[32];
    if (parse_string(buf, "language", code, sizeof code)) {
        if (strcmp(code, "de") == 0) s->lang = 1;
        else if (strcmp(code, "es") == 0) s->lang = 2;
        else if (strcmp(code, "fr") == 0) s->lang = 3;
        else if (strcmp(code, "it") == 0) s->lang = 4;
        else if (strcmp(code, "pt") == 0) s->lang = 5;
        else s->lang = 0;
    }
    char theme[32];
    if (parse_string(buf, "theme", theme, sizeof theme)) {
        s->dark = strcmp(theme, "dark") == 0;
    }
    parse_string(buf, "last_directory", s->last_directory, sizeof s->last_directory);

    parse_int(buf, "width", &s->window_width);
    parse_int(buf, "height", &s->window_height);
    parse_bool(buf, "maximized", &s->window_maximized);

    parse_bool(buf, "checkerboard", &s->viewer_checkerboard);
    {
        double d;
        if (parse_double(buf, "max_zoom", &d)) s->viewer_max_zoom = (float)d;
    }

    parse_bool(buf, "shuffle", &s->slideshow_shuffle);
    parse_bool(buf, "recursive", &s->slideshow_recursive);
    parse_bool(buf, "wrap", &s->slideshow_wrap);
    {
        double d;
        if (parse_double(buf, "interval_seconds", &d)) s->slideshow_interval_seconds = d;
    }

    parse_bool(buf, "enabled", &s->thumbnails_enabled);
    parse_string(buf, "location", s->thumbnails_location, sizeof s->thumbnails_location);
    parse_int(buf, "size", &s->thumbnails_size);
    parse_int(buf, "quality", &s->thumbnails_quality);
    parse_bool(buf, "show_directories", &s->thumbnails_show_directories);
    parse_bool(buf, "slow_update", &s->thumbnails_slow_update);

    if (s->thumbnails_location[0] == '\0') {
        snprintf(s->thumbnails_location, sizeof s->thumbnails_location, "per-directory");
    }
    if (s->slideshow_interval_seconds < 0.5) s->slideshow_interval_seconds = 0.5;
    if (s->thumbnails_size < 32) s->thumbnails_size = 128;
    if (s->viewer_max_zoom < 1.0f) s->viewer_max_zoom = 16.0f;

    free(buf);
    (void)v;
    return 0;
}

int settings_save(const app_settings *s, const char *path) {
    if (!s || !path || !path[0]) return -1;

    /* Ensure parent directory exists. */
    char dir[4096];
    snprintf(dir, sizeof dir, "%s", path);
    char *slash = strrchr(dir, '/');
    if (!slash) slash = strrchr(dir, '\\');
    if (slash) {
        *slash = '\0';
        mkdir_p(dir);
    }

    char tmp[4096];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return -1;

    const char *lang = "en";
    if (s->lang == 1) lang = "de";
    else if (s->lang == 2) lang = "es";
    else if (s->lang == 3) lang = "fr";
    else if (s->lang == 4) lang = "it";
    else if (s->lang == 5) lang = "pt";

    fprintf(f, "{\n");
    fprintf(f, "  \"language\": \"%s\",\n", lang);
    fprintf(f, "  \"theme\": \"%s\",\n", s->dark ? "dark" : "light");
    fprintf(f, "  \"last_directory\": \"%s\",\n", s->last_directory);
    fprintf(f, "  \"window\": {\n");
    fprintf(f, "    \"width\": %d,\n", s->window_width);
    fprintf(f, "    \"height\": %d,\n", s->window_height);
    fprintf(f, "    \"maximized\": %s\n", s->window_maximized ? "true" : "false");
    fprintf(f, "  },\n");
    fprintf(f, "  \"viewer\": {\n");
    fprintf(f, "    \"checkerboard\": %s,\n", s->viewer_checkerboard ? "true" : "false");
    fprintf(f, "    \"max_zoom\": %.3f\n", (double)s->viewer_max_zoom);
    fprintf(f, "  },\n");
    fprintf(f, "  \"slideshow\": {\n");
    fprintf(f, "    \"interval_seconds\": %.3f,\n", s->slideshow_interval_seconds);
    fprintf(f, "    \"shuffle\": %s,\n", s->slideshow_shuffle ? "true" : "false");
    fprintf(f, "    \"recursive\": %s,\n", s->slideshow_recursive ? "true" : "false");
    fprintf(f, "    \"wrap\": %s\n", s->slideshow_wrap ? "true" : "false");
    fprintf(f, "  },\n");
    fprintf(f, "  \"thumbnails\": {\n");
    fprintf(f, "    \"enabled\": %s,\n", s->thumbnails_enabled ? "true" : "false");
    fprintf(f, "    \"location\": \"%s\",\n", s->thumbnails_location);
    fprintf(f, "    \"size\": %d,\n", s->thumbnails_size);
    fprintf(f, "    \"quality\": %d,\n", s->thumbnails_quality);
    fprintf(f, "    \"show_directories\": %s,\n", s->thumbnails_show_directories ? "true" : "false");
    fprintf(f, "    \"slow_update\": %s\n", s->thumbnails_slow_update ? "true" : "false");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    fflush(f);
    if (ferror(f)) { fclose(f); remove(tmp); return -1; }
    fclose(f);

    remove(path);
    if (rename(tmp, path) != 0) { remove(tmp); return -1; }
    return 0;
}

char *settings_default_path(void) {
#ifdef _WIN32
    const char *base = getenv("APPDATA");
    if (!base) base = getenv("LOCALAPPDATA");
    if (!base) base = ".";
    char dir[4096];
    snprintf(dir, sizeof dir, "%s\\cuiviewer", base);
    mkdir_p(dir);
    char *out = (char *)malloc(strlen(dir) + 32);
    snprintf(out, strlen(dir) + 32, "%s\\settings.json", dir);
#else
    const char *base = getenv("XDG_DATA_HOME");
    char dirbuf[4096];
    if (!base || !base[0]) {
        const char *home = getenv("HOME");
        if (!home) home = ".";
        snprintf(dirbuf, sizeof dirbuf, "%s/.local/share", home);
        base = dirbuf;
    }
    char dir[4096];
    snprintf(dir, sizeof dir, "%s/cuiviewer", base);
    mkdir_p(dir);
    char *out = (char *)malloc(strlen(dir) + 32);
    snprintf(out, strlen(dir) + 32, "%s/settings.json", dir);
#endif
    return out;
}