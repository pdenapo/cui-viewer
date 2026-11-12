#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int lang;
    bool dark;

    char last_directory[4096];

    int window_width;
    int window_height;
    bool window_maximized;

    bool viewer_checkerboard;
    float viewer_max_zoom;

    bool slideshow_shuffle;
    bool slideshow_recursive;
    bool slideshow_wrap;
    double slideshow_interval_seconds;

    bool thumbnails_enabled;
    char thumbnails_location[16];
    int thumbnails_size;
    int thumbnails_quality;
    bool thumbnails_show_directories;
    bool thumbnails_slow_update;
    bool preview_enabled;
} app_settings;

void settings_defaults(app_settings *s);

/* Returns 0 on success. Missing/invalid files keep defaults and return 0. */
int settings_load(app_settings *s, const char *path);

/* Returns 0 on success. */
int settings_save(const app_settings *s, const char *path);

/* malloc'ed path to settings.json in the application data directory. */
char *settings_default_path(void);

#endif