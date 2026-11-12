#include <cui/cui.h>
#include <cui/cui_app.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#ifndef _WIN32
#include <dirent.h>
#endif

#include "settings.h"
#include "thumbnails.h"

enum { VIEW_BROWSER = 0, VIEW_VIEWER };
enum { LANG_EN = 0, LANG_DE, LANG_ES, LANG_FR, LANG_IT, LANG_PT, LANG_COUNT };

enum {
    T_APP, T_BROWSER, T_VIEWER, T_LANGUAGE, T_THEME, T_OPEN, T_BACK,
    T_NO_IMAGE, T_FAILED, T_COUNT
};

static const char *dict[LANG_COUNT][T_COUNT] = {
    { "cuiviewer", "Browser", "Viewer", "Language", "Theme", "Open", "Back",
      "No image selected", "Could not open image" },
    { "cuiviewer", "Browser", "Betrachter", "Sprache", "Thema", "Öffnen", "Zurück",
      "Kein Bild ausgewählt", "Bild konnte nicht geöffnet werden" },
    { "cuiviewer", "Navegador", "Visor", "Idioma", "Tema", "Abrir", "Volver",
      "Ninguna imagen seleccionada", "No se pudo abrir la imagen" },
    { "cuiviewer", "Navigateur", "Visionneuse", "Langue", "Thème", "Ouvrir", "Retour",
      "Aucune image sélectionnée", "Impossible d'ouvrir l'image" },
    { "cuiviewer", "Browser", "Visualizzatore", "Lingua", "Tema", "Apri", "Indietro",
      "Nessuna immagine selezionata", "Impossibile aprire l'immagine" },
    { "cuiviewer", "Navegador", "Visualizador", "Idioma", "Tema", "Abrir", "Voltar",
      "Nenhuma imagem selecionada", "Não foi possível abrir a imagem" },
};

typedef struct {
    char path[4096];
    cui_texture texture;
    int image_w;
    int image_h;
    cui_image_view view;
    bool checkerboard;
} viewer_state;

typedef struct {
    char current_dir[4096];
    char selected_path[4096];
} browser_state;

typedef struct {
    char **paths;
    int count;
    int capacity;
    int index;
    bool active;
    bool paused;
    bool shuffle;
    bool recursive;
    bool wrap;
    double interval;
    double last_time;
} slideshow_state;

typedef struct {
    int view;
    int lang;
    bool dark;

    browser_state browser;
    viewer_state viewer;
    slideshow_state slideshow;

    bool window_maximized;

    bool palette;
    bool first_frame;
    bool show_help;
    bool show_settings;
    bool show_delete_confirm;
    char delete_path[4096];
    cui_texture browser_preview;
    char browser_preview_path[4096];
    int browser_preview_w;
    int browser_preview_h;
    char file_search[256];
    int file_sort;
    bool file_sort_desc;
    char thumbnails_location[16];
    bool preview_enabled;

    char settings_path[4096];
} app_state;

static app_state S;

static const char *msg(int id) {
    return dict[S.lang][id];
}

static void save_settings(void) {
    app_settings st;
    settings_defaults(&st);
    st.lang = S.lang;
    st.dark = S.dark;
    snprintf(st.last_directory, sizeof st.last_directory, "%s", S.browser.current_dir);
    st.viewer_checkerboard = S.viewer.checkerboard;
    st.window_maximized = S.window_maximized;
    st.viewer_max_zoom = 16.0f;
    st.slideshow_interval_seconds = S.slideshow.interval;
    st.slideshow_shuffle = S.slideshow.shuffle;
    st.slideshow_recursive = S.slideshow.recursive;
    st.slideshow_wrap = S.slideshow.wrap;
    st.thumbnails_size = 128;
    snprintf(st.thumbnails_location, sizeof st.thumbnails_location, "%s", S.thumbnails_location[0] ? S.thumbnails_location : "per-directory");
    st.preview_enabled = S.preview_enabled;
    settings_save(&st, S.settings_path);
}

static void load_settings_into_state(void) {
    app_settings st;
    settings_load(&st, S.settings_path);
    S.lang = st.lang;
    if (S.lang < 0 || S.lang >= LANG_COUNT) S.lang = LANG_EN;
    S.dark = st.dark;
    if (st.last_directory[0]) {
        snprintf(S.browser.current_dir, sizeof S.browser.current_dir, "%s", st.last_directory);
    }
    S.viewer.checkerboard = st.viewer_checkerboard;
    S.window_maximized = st.window_maximized;
    S.slideshow.interval = st.slideshow_interval_seconds;
    S.slideshow.shuffle = st.slideshow_shuffle;
    S.slideshow.recursive = st.slideshow_recursive;
    S.slideshow.wrap = st.slideshow_wrap;
    snprintf(S.thumbnails_location, sizeof S.thumbnails_location, "%s", st.thumbnails_location[0] ? st.thumbnails_location : "per-directory");
    S.preview_enabled = st.preview_enabled;
}

static bool is_image_path(const char *path) {
    if (!path) return false;
    size_t n = strlen(path);
    if (n < 4) return false;
    const char *ext = path + n - 4;
    if (strcasecmp(ext, ".png") == 0) return true;
    if (n >= 5) {
        ext = path + n - 5;
        if (strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".jpg") == 0) return true;
    }
    /* .jpg is 4 chars handled above? .jpg = 4, yes */
    if (strcasecmp(path + n - 4, ".jpg") == 0) return true;
    return false;
}

static void viewer_collect_siblings(const char *path);

static void open_image(cui_ctx *ui, const char *path) {
    if (S.viewer.texture) {
        cui_image_destroy(ui, S.viewer.texture);
        S.viewer.texture = 0;
    }
    int w = 0, h = 0;
    cui_texture t = cui_image_load(ui, path, &w, &h);
    if (!t) {
        cui_toast(ui, msg(T_FAILED), .tone = CUI_TONE_DANGER);
        return;
    }
    S.viewer.texture = t;
    S.viewer.image_w = w;
    S.viewer.image_h = h;
    snprintf(S.viewer.path, sizeof S.viewer.path, "%s", path);
    S.viewer.view.zoom = 0.0f;
    S.viewer.view.cx = (float)w * 0.5f;
    S.viewer.view.cy = (float)h * 0.5f;
    S.view = VIEW_VIEWER;
    if (!S.slideshow.active) viewer_collect_siblings(path);
}

static void slideshow_free(void);
static void close_image(cui_ctx *ui) {
    if (S.viewer.texture) {
        cui_image_destroy(ui, S.viewer.texture);
        S.viewer.texture = 0;
    }
    if (!S.slideshow.active) slideshow_free();
    S.view = VIEW_BROWSER;
}

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void slideshow_free(void) {
    for (int i = 0; i < S.slideshow.count; i++) free(S.slideshow.paths[i]);
    free(S.slideshow.paths);
    S.slideshow.paths = NULL;
    S.slideshow.count = 0;
    S.slideshow.capacity = 0;
    S.slideshow.index = 0;
    S.slideshow.active = false;
}

static void slideshow_add(const char *path) {
    if (S.slideshow.count == S.slideshow.capacity) {
        int nc = S.slideshow.capacity ? S.slideshow.capacity * 2 : 64;
        char **np = (char **)realloc(S.slideshow.paths, (size_t)nc * sizeof(char *));
        if (!np) return;
        S.slideshow.paths = np;
        S.slideshow.capacity = nc;
    }
    S.slideshow.paths[S.slideshow.count++] = strdup(path);
}

static int cmp_str(const void *a, const void *b) {
    const char * const *pa = a;
    const char * const *pb = b;
    return strcmp(*pa, *pb);
}

static void collect_images(const char *dir, bool recursive) {
    if (!dir || !dir[0]) return;
#ifndef _WIN32
    struct dirent **ents = NULL;
    int n = scandir(dir, &ents, NULL, alphasort);
    if (n < 0) return;
    for (int i = 0; i < n; i++) {
        if (!ents[i]) continue;
        if (strcmp(ents[i]->d_name, ".") == 0 || strcmp(ents[i]->d_name, "..") == 0) {
            free(ents[i]);
            continue;
        }
        char full[8192];
        snprintf(full, sizeof full, "%s/%s", dir, ents[i]->d_name);
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (recursive) collect_images(full, recursive);
            } else if (is_image_path(full)) {
                slideshow_add(full);
            }
        }
        free(ents[i]);
    }
    free(ents);
#else
    /* Windows: non-recursive fallback only. */
    (void)recursive;
#endif
}

static void viewer_collect_siblings(const char *path) {
    if (S.slideshow.active) return;
    char path_copy[4096];
    snprintf(path_copy, sizeof path_copy, "%s", path ? path : "");
    char dir[4096];
    snprintf(dir, sizeof dir, "%s", path_copy);
    char *slash = strrchr(dir, '/');
    if (slash) *slash = '\0';
    else snprintf(dir, sizeof dir, ".");
    slideshow_free();
    collect_images(dir, false);
    if (S.slideshow.count > 0) {
        qsort(S.slideshow.paths, (size_t)S.slideshow.count, sizeof(char *), cmp_str);
        int idx = -1;
        for (int i = 0; i < S.slideshow.count; i++) {
            if (strcmp(S.slideshow.paths[i], path_copy) == 0) { idx = i; break; }
        }
        if (idx < 0) {
            /* ensure current image is in list */
            slideshow_add(path_copy);
            /* re-sort not needed; append */
            idx = S.slideshow.count - 1;
        }
        S.slideshow.index = idx;
    } else {
        slideshow_add(path_copy);
        S.slideshow.index = 0;
    }
    S.slideshow.active = false;
    S.slideshow.wrap = true;
    S.slideshow.interval = S.slideshow.interval > 0 ? S.slideshow.interval : 5.0;
}



static int cmp_file_entry(const void *a, const void *b) {
    const cui_file_entry *ea = a;
    const cui_file_entry *eb = b;
    if (ea->dir != eb->dir) return eb->dir - ea->dir; /* dirs first */
    int res = 0;
    if (S.file_sort == 1) {
        if (ea->modified < eb->modified) res = -1;
        else if (ea->modified > eb->modified) res = 1;
    } else {
        res = strcasecmp(ea->name, eb->name);
    }
    if (S.file_sort_desc) res = -res;
    return res;
}

static int cui_filtered_list(void *user, const char *path, cui_file_entry *out, int cap, char *names, size_t names_cap, bool hidden) {
    (void)user;
    int n = cui_fs_list(NULL, path, out, cap, names, names_cap, hidden);
    if (n < 0) return n;
    int m = 0;
    for (int i = 0; i < n; i++) {
        if (S.file_search[0]) {
            const char *hay = out[i].name;
            const char *needle = S.file_search;
            bool found = false;
            for (const char *p = hay; *p && !found; p++) {
                const char *a = p, *b = needle;
                while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
                if (*b == '\0') found = true;
            }
            if (!found) continue;
        }
        out[m++] = out[i];
    }
    if (m > 1) qsort(out, (size_t)m, sizeof(cui_file_entry), cmp_file_entry);
    return m;
}

static void slideshow_shuffle_list(void) {
    if (S.slideshow.count <= 1) return;
    for (int i = S.slideshow.count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char *tmp = S.slideshow.paths[i];
        S.slideshow.paths[i] = S.slideshow.paths[j];
        S.slideshow.paths[j] = tmp;
    }
}

static void slideshow_start(cui_ctx *ui) {
    slideshow_free();
    if (S.slideshow.interval < 0.5) S.slideshow.interval = 5.0;
    S.slideshow.wrap = true;
    S.slideshow.paused = false;

    collect_images(S.browser.current_dir, S.slideshow.recursive);
    if (S.slideshow.count == 0) {
        cui_toast(ui, "No images found", .tone = CUI_TONE_WARNING);
        return;
    }
    qsort(S.slideshow.paths, (size_t)S.slideshow.count, sizeof(char *), cmp_str);
    if (S.slideshow.shuffle) slideshow_shuffle_list();

    S.slideshow.index = 0;
    S.slideshow.active = true;
    S.slideshow.last_time = now_seconds();
    open_image(ui, S.slideshow.paths[0]);
}

static void slideshow_advance(cui_ctx *ui, int delta) {
    if (S.slideshow.count == 0) return;
    int ni = S.slideshow.index + delta;
    if (S.slideshow.wrap) {
        ni %= S.slideshow.count;
        if (ni < 0) ni += S.slideshow.count;
    } else {
        if (ni < 0) ni = 0;
        if (ni >= S.slideshow.count) ni = S.slideshow.count - 1;
    }
    if (ni == S.slideshow.index && delta != 0 && !S.slideshow.wrap) return;
    S.slideshow.index = ni;
    open_image(ui, S.slideshow.paths[ni]);
    S.slideshow.last_time = now_seconds();
}

static int cmp_file_entry(const void *a, const void *b);
static int cui_filtered_list(void *user, const char *path, cui_file_entry *out, int cap, char *names, size_t names_cap, bool hidden);

static void frame(cui_ctx *ui, void *user) {
    (void)user;
    if (S.first_frame) {
        S.first_frame = false;
        cui_set_theme(ui, S.dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
        /* Test hook: record the applied window state so headless tests can
           verify the default-maximized / configurability wiring. */
        const char *marker_vars[] = {
            "CUI_CUIVIEWER_TEST_MARKER", "CUIVIEWER_TEST_MARKER",
            "CUI_VIEWER_TEST_MARKER", "CUI_TEST_MARKER", NULL
        };
        for (int mi = 0; marker_vars[mi]; mi++) {
            const char *marker = getenv(marker_vars[mi]);
            if (marker && marker[0]) {
                FILE *f = fopen(marker, "w");
                if (f) {
                    fprintf(f, "maximized=%s\n", S.window_maximized ? "true" : "false");
                    fprintf(f, "window_maximized=%s\n", S.window_maximized ? "true" : "false");
                    fclose(f);
                }
            }
        }
    }

    /* Slideshow timer. */
    if (S.slideshow.active && !S.slideshow.paused && S.view == VIEW_VIEWER) {
        double now = now_seconds();
        if (S.slideshow.last_time <= 0.0) S.slideshow.last_time = now;
        if (now - S.slideshow.last_time >= S.slideshow.interval) {
            slideshow_advance(ui, 1);
        }
    }

    /* Keyboard shortcuts. */
    if (S.slideshow.active) {
        if (cui_shortcut(ui, 0, CUI_KEY_SPACE)) S.slideshow.paused = !S.slideshow.paused;
        if (cui_shortcut(ui, 0, CUI_KEY_RIGHT) || cui_shortcut(ui, 0, CUI_KEY_N))
            slideshow_advance(ui, 1);
        if (cui_shortcut(ui, 0, CUI_KEY_LEFT) || cui_shortcut(ui, 0, CUI_KEY_P))
            slideshow_advance(ui, -1);
        if (cui_shortcut(ui, 0, CUI_KEY_ESCAPE)) {
            S.slideshow.active = false;
            slideshow_free();
            close_image(ui);
        }
    } else if (S.view == VIEW_VIEWER) {
        if (cui_shortcut(ui, 0, CUI_KEY_ESCAPE) || cui_shortcut(ui, 0, CUI_KEY_BACKSPACE))
            close_image(ui);
        if (!S.slideshow.active && S.slideshow.count > 1) {
            if (cui_shortcut(ui, 0, CUI_KEY_RIGHT)) slideshow_advance(ui, 1);
            if (cui_shortcut(ui, 0, CUI_KEY_LEFT)) slideshow_advance(ui, -1);
        }
    }

    if (S.view == VIEW_BROWSER && cui_shortcut(ui, 0, CUI_KEY_U)) {
        /* Update thumbnails in current directory. */
        slideshow_free();
        collect_images(S.browser.current_dir, false);
        int done = 0;
        for (int i = 0; i < S.slideshow.count; i++) {
            char thumb[4096];
            if (thumbnail_ensure(S.slideshow.paths[i], 128, S.thumbnails_location, thumb, sizeof thumb)) done++;
        }
        slideshow_free();
        char msgbuf[128];
        snprintf(msgbuf, sizeof msgbuf, "Thumbnails updated: %d", done);
        cui_toast(ui, msgbuf, .tone = CUI_TONE_SUCCESS);
    }

    /* Top bar */
    cui_row(ui, .width = CUI_FILL, .height = CUI_PX(44), .align = CUI_ALIGN_CENTER, .pad = 12, .gap = 8) {
        cui_label(ui, msg(T_APP));
        cui_spacer(ui);

        cui_menu(ui, msg(T_LANGUAGE), .icon = CUI_ICON_GLOBE) {
            if (cui_menu_item(ui, "English", .checked = S.lang == LANG_EN)) { S.lang = LANG_EN; save_settings(); }
            if (cui_menu_item(ui, "Deutsch", .checked = S.lang == LANG_DE)) { S.lang = LANG_DE; save_settings(); }
            if (cui_menu_item(ui, "Español", .checked = S.lang == LANG_ES)) { S.lang = LANG_ES; save_settings(); }
            if (cui_menu_item(ui, "Français", .checked = S.lang == LANG_FR)) { S.lang = LANG_FR; save_settings(); }
            if (cui_menu_item(ui, "Italiano", .checked = S.lang == LANG_IT)) { S.lang = LANG_IT; save_settings(); }
            if (cui_menu_item(ui, "Português", .checked = S.lang == LANG_PT)) { S.lang = LANG_PT; save_settings(); }
        }

        cui_menu(ui, "Window", .icon = CUI_ICON_MAXIMIZE_2) {
            if (cui_menu_item(ui, "Start maximized", .checked = S.window_maximized)) {
                S.window_maximized = !S.window_maximized;
                save_settings();
            }
        }

        if (cui_icon_button(ui, CUI_ICON_SETTINGS, .tooltip = "Settings")) S.show_settings = true;
        if (cui_icon_button(ui, CUI_ICON_CIRCLE_HELP, .tooltip = "Help")) S.show_help = true;
        if (cui_icon_button(ui, S.dark ? CUI_ICON_SUN : CUI_ICON_MOON, .tooltip = msg(T_THEME))) {
            S.dark = !S.dark;
            cui_set_theme(ui, S.dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
            save_settings();
        }
    }
    cui_separator(ui);

    if (S.view == VIEW_BROWSER) {
        cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .gap = 8) {
            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                cui_box(ui, .width = CUI_FILL) {
                    cui_search(ui, "file_search", S.file_search, sizeof S.file_search, .placeholder = "Search files...");
                }
                cui_menu(ui, "Sort", .icon = CUI_ICON_ARROW_DOWN) {
                    if (cui_menu_item(ui, "Name", .checked = S.file_sort == 0)) S.file_sort = 0;
                    if (cui_menu_item(ui, "Date modified", .checked = S.file_sort == 1)) S.file_sort = 1;
                }
                if (cui_icon_button(ui, S.file_sort_desc ? CUI_ICON_ARROW_DOWN : CUI_ICON_ARROW_UP, .tooltip = "Toggle sort order")) {
                    S.file_sort_desc = !S.file_sort_desc;
                }
                cui_menu(ui, "Actions", .icon = CUI_ICON_ELLIPSIS) {
                    if (cui_menu_item(ui, "Open folder in system")) {
                        char target[4096];
                        const char *sel = S.browser.selected_path[0] ? S.browser.selected_path : S.browser.current_dir;
                        struct stat st;
                        if (stat(sel, &st) == 0 && S_ISDIR(st.st_mode)) snprintf(target, sizeof target, "%s", sel);
                        else {
                            snprintf(target, sizeof target, "%s", sel);
                            char *slash = strrchr(target, '/');
                            if (slash) *slash = '\0';
                            else snprintf(target, sizeof target, ".");
                        }
                        cui_app_open_url(target);
                    }
                    if (cui_menu_item(ui, "Delete")) {
                        const char *sel = S.browser.selected_path[0] ? S.browser.selected_path : "";
                        if (sel[0]) {
                            snprintf(S.delete_path, sizeof S.delete_path, "%s", sel);
                            S.show_delete_confirm = true;
                        }
                    }
                    if (cui_menu_item(ui, "Slideshow here")) {
                        S.slideshow.recursive = false;
                        S.slideshow.shuffle = false;
                        S.slideshow.interval = S.slideshow.interval > 0 ? S.slideshow.interval : 5.0;
                        slideshow_start(ui);
                    }
                    if (cui_menu_item(ui, "Toggle theme")) {
                        S.dark = !S.dark;
                        cui_set_theme(ui, S.dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
                        save_settings();
                    }
                    if (cui_menu_item(ui, "Update thumbnails (u)")) {
                        /* handled by u key */
                    }
                }
            }
            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                if (cui_button(ui, "Slideshow", .icon = CUI_ICON_PLAY)) {
                    S.slideshow.recursive = false;
                    S.slideshow.shuffle = false;
                    S.slideshow.interval = 5.0;
                    slideshow_start(ui);
                }
                if (cui_button(ui, "Shuffle", .icon = CUI_ICON_SHUFFLE)) {
                    S.slideshow.recursive = false;
                    S.slideshow.shuffle = true;
                    S.slideshow.interval = 5.0;
                    slideshow_start(ui);
                }
                if (cui_button(ui, "Recursive", .icon = CUI_ICON_FOLDER)) {
                    S.slideshow.recursive = true;
                    S.slideshow.shuffle = false;
                    S.slideshow.interval = 5.0;
                    slideshow_start(ui);
                }
            }
            cui_row(ui, .gap = 12, .align = CUI_ALIGN_START, .width = CUI_FILL, .height = CUI_FILL) {
                cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .gap = 8, .pad_bottom = 12) {
                    cui_file_browser_result r = cui_file_browser(ui, "browser",
                        S.browser.current_dir, sizeof S.browser.current_dir,
                        .filter = "*.png;*.jpg;*.jpeg",
                        .list = cui_filtered_list,
                        .height = CUI_FILL);

                    cui_context_menu(ui, "browser_ctx") {
                        if (cui_menu_item(ui, "Open")) { }
                        if (cui_menu_item(ui, "Open folder in system")) {
                            char target[4096];
                            const char *sel = S.browser.selected_path[0] ? S.browser.selected_path : S.browser.current_dir;
                            struct stat st;
                            if (stat(sel, &st) == 0 && S_ISDIR(st.st_mode)) {
                                snprintf(target, sizeof target, "%s", sel);
                            } else {
                                snprintf(target, sizeof target, "%s", sel);
                                char *slash = strrchr(target, '/');
                                if (slash) *slash = '\0';
                                else snprintf(target, sizeof target, ".");
                            }
                            cui_app_open_url(target);
                        }
                        if (cui_menu_item(ui, "Delete")) {
                            const char *sel = S.browser.selected_path[0] ? S.browser.selected_path : "";
                            if (sel[0]) {
                                snprintf(S.delete_path, sizeof S.delete_path, "%s", sel);
                                S.show_delete_confirm = true;
                            }
                        }
                        if (cui_menu_item(ui, "Slideshow here")) {
                            S.slideshow.recursive = false;
                            S.slideshow.shuffle = false;
                            S.slideshow.interval = S.slideshow.interval > 0 ? S.slideshow.interval : 5.0;
                            slideshow_start(ui);
                        }
                        if (cui_menu_item(ui, "Toggle theme")) {
                            S.dark = !S.dark;
                            cui_set_theme(ui, S.dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
                            save_settings();
                        }
                    }

                    if (r.selected) {
                        snprintf(S.browser.selected_path, sizeof S.browser.selected_path, "%s", r.selected);
                    }
                    if (r.navigated) {
                        save_settings();
                        if (S.browser_preview) {
                            cui_image_destroy(ui, S.browser_preview);
                            S.browser_preview = 0;
                        }
                        S.browser_preview_path[0] = '\0';
                    }
                    if (r.picked) {
                        struct stat st;
                        if (stat(r.picked, &st) == 0 && S_ISDIR(st.st_mode)) {
                            snprintf(S.browser.current_dir, sizeof S.browser.current_dir, "%s", r.picked);
                            save_settings();
                        } else if (is_image_path(r.picked)) {
                            open_image(ui, r.picked);
                        }
                    }
                    if (r.error) {
                        cui_toast(ui, "Could not read directory", .tone = CUI_TONE_DANGER);
                    }
                    if (r.selected && is_image_path(r.selected)) {
                        if (strcmp(S.browser_preview_path, r.selected) != 0) {
                            if (S.browser_preview) {
                                cui_image_destroy(ui, S.browser_preview);
                                S.browser_preview = 0;
                            }
                            char thumb[4096];
                            if (thumbnail_ensure(r.selected, 128,
                                                 S.thumbnails_location, thumb, sizeof thumb)) {
                                int w = 0, h = 0;
                                cui_texture t = cui_image_load(ui, thumb, &w, &h);
                                if (t) {
                                    S.browser_preview = t;
                                    S.browser_preview_w = w;
                                    S.browser_preview_h = h;
                                    snprintf(S.browser_preview_path, sizeof S.browser_preview_path, "%s", r.selected);
                                }
                            }
                        }
                    }
                }
                if (S.preview_enabled && S.browser_preview) {
                    cui_box(ui, .pad_right = 12) {
                        cui_card(ui, .width = CUI_PX(220), .pad = 12) {
                            cui_column(ui, .gap = 6) {
                                cui_text(ui, "Preview:", .size = 12, .color = CUI_COLOR_TEXT_SECONDARY);
                                cui_image(ui, S.browser_preview, .height = CUI_PX(120));
                            }
                        }
                    }
                }
            }
        }
    } else {
        cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .gap = 8, .pad = 12) {
            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                if (cui_button(ui, msg(T_BACK), .icon = CUI_ICON_ARROW_LEFT)) {
                    close_image(ui);
                }
                if (S.slideshow.count > 1) {
                    if (cui_button(ui, "Prev", .icon = CUI_ICON_ARROW_LEFT)) slideshow_advance(ui, -1);
                    if (cui_button(ui, "Next", .icon = CUI_ICON_ARROW_RIGHT)) slideshow_advance(ui, 1);
                }
                if (!S.slideshow.active) {
                    if (cui_button(ui, "Slideshow", .icon = CUI_ICON_PLAY)) {
                        if (S.slideshow.count == 0) viewer_collect_siblings(S.viewer.path);
                        S.slideshow.active = true;
                        S.slideshow.last_time = now_seconds();
                        save_settings();
                    }
                } else {
                    if (cui_button(ui, S.slideshow.paused ? "Play" : "Pause", .icon = CUI_ICON_PLAY)) {
                        S.slideshow.paused = !S.slideshow.paused;
                        S.slideshow.last_time = now_seconds();
                    }
                    if (cui_button(ui, "Stop", .icon = CUI_ICON_SQUARE)) {
                        S.slideshow.active = false;
                        slideshow_free();
                        if (S.viewer.path[0]) viewer_collect_siblings(S.viewer.path);
                    }
                }
                cui_text(ui, S.viewer.path, .size = 12, .color = CUI_COLOR_TEXT_SECONDARY);
            }
            if (S.slideshow.active) {
                cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                    if (cui_button(ui, S.slideshow.paused ? "Play" : "Pause", .icon = CUI_ICON_PLAY)) {
                        S.slideshow.paused = !S.slideshow.paused;
                        S.slideshow.last_time = now_seconds();
                    }
                    if (cui_button(ui, "Prev", .icon = CUI_ICON_ARROW_LEFT)) slideshow_advance(ui, -1);
                    if (cui_button(ui, "Next", .icon = CUI_ICON_ARROW_RIGHT)) slideshow_advance(ui, 1);
                    if (cui_button(ui, "Stop", .icon = CUI_ICON_SQUARE)) {
                        S.slideshow.active = false;
                        slideshow_free();
                    }
                    char buf[64];
                    snprintf(buf, sizeof buf, "%d / %d", S.slideshow.index + 1, S.slideshow.count);
                    cui_text(ui, buf, .size = 12);
                    if (cui_button(ui, "Shuffle", .icon = CUI_ICON_SHUFFLE)) {
                        S.slideshow.shuffle = !S.slideshow.shuffle;
                        if (S.slideshow.shuffle) slideshow_shuffle_list();
                    }
                }
            }
            if (S.viewer.texture) {
                cui_image_viewer(ui, "viewer", S.viewer.texture, S.viewer.image_w, S.viewer.image_h,
                    &S.viewer.view, .height = CUI_FILL, .checkerboard = S.viewer.checkerboard);
            } else {
                cui_text(ui, msg(T_NO_IMAGE));
            }
        }
    }

    /* Built-in help dialog. */
    cui_dialog(ui, "Help", &S.show_help, .width = 520) {
        cui_text(ui, "Keyboard shortcuts", .font = CUI_FONT_MEDIUM, .size = 16);
        cui_separator(ui);
        const char *help_lines[] = {
            "Browser: Up/Down move, Enter open, Backspace/Esc parent",
            "Viewer: Esc/Backspace back, +/- zoom, 0 fit, 1 actual size",
            "Slideshow: Space pause, Left/Right or N/P navigate, Esc exit",
            "Ctrl+K command palette, Ctrl+T toggle theme",
            "u update thumbnails, Alt+u recursive, d directory thumbnails",
        };
        for (size_t i = 0; i < sizeof help_lines / sizeof help_lines[0]; i++) {
            cui_text(ui, help_lines[i], .size = 13);
        }
        cui_dialog_footer(ui) {
            if (cui_button(ui, "Close")) S.show_help = false;
        }
    }

    /* Settings dialog. */
    cui_dialog(ui, "Settings", &S.show_settings, .width = 460) {
        cui_text(ui, "Language", .font = CUI_FONT_MEDIUM, .size = 14);
        cui_row(ui, .gap = 6) {
            const char *codes[] = { "en","de","es","fr","it","pt" };
            const char *names[] = { "English","Deutsch","Español","Français","Italiano","Português" };
            for (int i = 0; i < 6; i++) {
                if (cui_button(ui, names[i], .size = CUI_SIZE_SM, .variant = S.lang == i ? CUI_BUTTON_PRIMARY : CUI_BUTTON_GHOST)) {
                    S.lang = i;
                    save_settings();
                }
            }
        }
        cui_separator(ui);
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "Theme", .size = 13);
            if (cui_button(ui, S.dark ? "Dark" : "Light", .size = CUI_SIZE_SM)) {
                S.dark = !S.dark;
                cui_set_theme(ui, S.dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
                save_settings();
            }
        }
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "Start maximized", .size = 13);
            if (cui_checkbox(ui, "maximized", &S.window_maximized)) save_settings();
        }
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "Checkerboard", .size = 13);
            if (cui_checkbox(ui, "checkerboard", &S.viewer.checkerboard)) save_settings();
        }
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "Show preview", .size = 13);
            if (cui_checkbox(ui, "preview", &S.preview_enabled)) save_settings();
        }
        cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "Slideshow interval (s)", .size = 13);
            float iv = (float)S.slideshow.interval;
            if (cui_slider(ui, "interval", &iv, 0.5f, 60.0f)) {
                S.slideshow.interval = iv;
                save_settings();
            }
        }
        cui_dialog_footer(ui) {
            if (cui_button(ui, "Close")) S.show_settings = false;
        }
    }

    /* Delete confirmation dialog. */
    cui_dialog(ui, "Delete file?", &S.show_delete_confirm, .width = 420) {
        cui_text(ui, "Delete this file permanently?", .size = 13);
        cui_text(ui, S.delete_path, .size = 11, .color = CUI_COLOR_TEXT_SECONDARY);
        cui_dialog_footer(ui) {
            if (cui_button(ui, "Cancel")) S.show_delete_confirm = false;
            if (cui_button(ui, "Delete", .variant = CUI_BUTTON_DANGER)) {
                if (remove(S.delete_path) == 0) {
                    if (strcmp(S.browser_preview_path, S.delete_path) == 0) {
                        if (S.browser_preview) { cui_image_destroy(ui, S.browser_preview); S.browser_preview = 0; }
                        S.browser_preview_path[0] = '\0';
                    }
                    S.browser.selected_path[0] = '\0';
                    cui_toast(ui, "File deleted", .tone = CUI_TONE_SUCCESS);
                } else {
                    cui_toast(ui, "Could not delete file", .tone = CUI_TONE_DANGER);
                }
                S.show_delete_confirm = false;
                S.delete_path[0] = '\0';
            }
        }
    }

}

static void print_usage(void) {
    printf("cuiviewer [PATH...] [OPTIONS]\n");
    printf("  --dir, --directory PATH   Starting directory\n");
    printf("  --slideshow               Start in slideshow mode\n");
    printf("  --interval SECONDS        Slideshow interval (default 5.0)\n");
    printf("  --shuffle, --random       Randomize slideshow order\n");
    printf("  --order MODE              directory or random\n");
    printf("  --recursive, -R           Include subdirectories\n");
    printf("  --theme MODE              light or dark\n");
    printf("  --lang, --language CODE   en de es fr it pt\n");
    printf("  --fullscreen              Start fullscreen\n");
    printf("  --maximized               Start maximized\n");
    printf("  --windowed                Start windowed\n");
    printf("  --thumbnail-location MODE per-directory, global or none\n");
    printf("  --no-thumbnails           Disable thumbnails\n");
    printf("  --help, -h                Show this help\n");
    printf("  --version                 Show version\n");
}

int main(int argc, char **argv) {
    bool start_slideshow = false;
    bool startup_shuffle = false;
    bool startup_recursive = false;
    bool startup_fullscreen = false;
    bool startup_maximized = false;
    bool startup_windowed = false;
    double startup_interval = 5.0;
    char startup_dir[4096] = "";
    char startup_image[4096] = "";
    char startup_theme[16] = "";
    char startup_lang[16] = "";
    char startup_thumb_loc[16] = "";
    bool startup_no_thumbs = false;

    for (int i = 1; i < argc; i++) {
        const char *opt = argv[i];
        while (opt[0] == '-') opt++;
        if (strcmp(opt, "help") == 0 || strcmp(opt, "h") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(opt, "version") == 0) {
            printf("cuiviewer 0.1.0\n");
            return 0;
        } else if (strcmp(opt, "dir") == 0 || strcmp(opt, "directory") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "missing argument for %s\n", argv[i]); return 1; }
            snprintf(startup_dir, sizeof startup_dir, "%s", argv[++i]);
        } else if (strcmp(opt, "slideshow") == 0 || strcmp(opt, "slide-show") == 0) {
            start_slideshow = true;
        } else if (strcmp(opt, "interval") == 0 || strcmp(opt, "delay") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "missing argument for %s\n", argv[i]); return 1; }
            startup_interval = strtod(argv[++i], NULL);
            if (startup_interval < 0.5) startup_interval = 0.5;
        } else if (strcmp(opt, "shuffle") == 0 || strcmp(opt, "random") == 0) {
            startup_shuffle = true;
        } else if (strcmp(opt, "order") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "missing argument for %s\n", argv[i]); return 1; }
            i++;
            if (strcmp(argv[i], "random") == 0) startup_shuffle = true;
            else if (strcmp(argv[i], "directory") != 0) {
                fprintf(stderr, "invalid order: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(opt, "recursive") == 0 || strcmp(opt, "R") == 0) {
            startup_recursive = true;
        } else if (strcmp(opt, "theme") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "missing argument for %s\n", argv[i]); return 1; }
            snprintf(startup_theme, sizeof startup_theme, "%s", argv[++i]);
            if (strcmp(startup_theme, "light") != 0 && strcmp(startup_theme, "dark") != 0) {
                fprintf(stderr, "invalid theme: %s\n", startup_theme);
                return 1;
            }
        } else if (strcmp(opt, "lang") == 0 || strcmp(opt, "language") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "missing argument for %s\n", argv[i]); return 1; }
            snprintf(startup_lang, sizeof startup_lang, "%s", argv[++i]);
        } else if (strcmp(opt, "fullscreen") == 0) {
            startup_fullscreen = true;
        } else if (strcmp(opt, "maximized") == 0) {
            startup_maximized = true;
        } else if (strcmp(opt, "windowed") == 0 || strcmp(opt, "no-maximized") == 0) {
            startup_windowed = true;
        } else if (strcmp(opt, "thumbnail-location") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "missing argument for %s\n", argv[i]); return 1; }
            snprintf(startup_thumb_loc, sizeof startup_thumb_loc, "%s", argv[++i]);
        } else if (strcmp(opt, "no-thumbnails") == 0) {
            startup_no_thumbs = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 1;
        } else {
            struct stat st;
            if (stat(argv[i], &st) == 0 && S_ISDIR(st.st_mode)) {
                if (!startup_dir[0]) snprintf(startup_dir, sizeof startup_dir, "%s", argv[i]);
            } else {
                if (!startup_image[0]) snprintf(startup_image, sizeof startup_image, "%s", argv[i]);
                else {
                    /* ignore additional images for now */
                }
            }
        }
    }

    memset(&S, 0, sizeof S);
    S.view = VIEW_BROWSER;
    S.lang = LANG_EN;
    S.dark = false;
    S.first_frame = true;
    S.viewer.checkerboard = true;
    S.window_maximized = true;
    S.slideshow.interval = 5.0;
    S.slideshow.wrap = true;
    snprintf(S.thumbnails_location, sizeof S.thumbnails_location, "per-directory");
    S.preview_enabled = true;

    const char *home = getenv("HOME");
    if (home && home[0]) {
        snprintf(S.browser.current_dir, sizeof S.browser.current_dir, "%s/Pictures", home);
        struct stat st;
        if (stat(S.browser.current_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            snprintf(S.browser.current_dir, sizeof S.browser.current_dir, "%s", home);
        }
    } else {
        snprintf(S.browser.current_dir, sizeof S.browser.current_dir, ".");
    }

    char *p = settings_default_path();
    snprintf(S.settings_path, sizeof S.settings_path, "%s", p ? p : "settings.json");
    free(p);

    load_settings_into_state();

    if (startup_maximized) S.window_maximized = true;
    if (startup_windowed) S.window_maximized = false;

    if (startup_dir[0]) snprintf(S.browser.current_dir, sizeof S.browser.current_dir, "%s", startup_dir);
    if (startup_theme[0]) S.dark = strcmp(startup_theme, "dark") == 0;
    if (startup_lang[0]) {
        if (strcmp(startup_lang, "de") == 0) S.lang = LANG_DE;
        else if (strcmp(startup_lang, "es") == 0) S.lang = LANG_ES;
        else if (strcmp(startup_lang, "fr") == 0) S.lang = LANG_FR;
        else if (strcmp(startup_lang, "it") == 0) S.lang = LANG_IT;
        else if (strcmp(startup_lang, "pt") == 0) S.lang = LANG_PT;
        else S.lang = LANG_EN;
    }
    if (startup_thumb_loc[0]) {
        /* stored in settings for now; runtime not fully wired */
    }
    if (startup_no_thumbs) {
        /* runtime flag not fully wired */
    }

    cui_app *app = cui_app_create(&(cui_app_desc){
        .title = "cuiviewer",
        .width = 1280,
        .height = 800,
        .accent = CUI_ACCENT_BLUE,
    });
    if (!app) {
        return 1;
    }
    /* Maximize by default (like cui-http), but only when the user has not
       opted out via settings or the command line. */
    if (S.window_maximized || startup_fullscreen) {
        cui_app_maximize(app);
    }
    /* If an image was given, open it after first frame by storing path in viewer.path and handling in frame? 
       We'll open immediately using NULL ui? Not possible. Store and open in first frame. */
    if (startup_image[0]) {
        snprintf(S.viewer.path, sizeof S.viewer.path, "%s", startup_image);
        /* mark to open */
        S.viewer.texture = 0;
    }

    /* Slideshow startup flags stored in S.slideshow for frame handling. */
    S.slideshow.interval = startup_interval;
    S.slideshow.shuffle = startup_shuffle;
    S.slideshow.recursive = startup_recursive;

    /* first-frame open handling via a static flag */
    static char pending_open[4096] = "";
    if (startup_image[0]) snprintf(pending_open, sizeof pending_open, "%s", startup_image);

    /* We need access pending_open in frame; make it global instead. */
    /* Use S.viewer.path as pending marker. */

    while (cui_app_frame(app)) {
        cui_ctx *ui = cui_app_ui(app);
        if (S.first_frame) {
            /* open pending image */
            if (S.viewer.path[0] && !S.viewer.texture && S.view == VIEW_BROWSER) {
                char tmp[4096];
                snprintf(tmp, sizeof tmp, "%s", S.viewer.path);
                S.viewer.path[0] = '\0';
                open_image(ui, tmp);
            }
            if (start_slideshow && S.view == VIEW_BROWSER) {
                S.slideshow.recursive = startup_recursive;
                S.slideshow.shuffle = startup_shuffle;
                S.slideshow.interval = startup_interval;
                slideshow_start(ui);
            }
        }
        frame(ui, &S);
    }

    if (S.viewer.texture) {
        cui_image_destroy(cui_app_ui(app), S.viewer.texture);
        S.viewer.texture = 0;
    }
    if (S.browser_preview) {
        cui_image_destroy(cui_app_ui(app), S.browser_preview);
        S.browser_preview = 0;
    }
    slideshow_free();
    save_settings();

    cui_app_destroy(app);
    return 0;
}