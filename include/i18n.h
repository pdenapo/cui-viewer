#ifndef I18N_H
#define I18N_H

#include <stddef.h>

enum { LANG_EN = 0, LANG_DE, LANG_ES, LANG_FR, LANG_IT, LANG_PT, LANG_COUNT };

enum {
    T_APP, T_BROWSER, T_VIEWER, T_LANGUAGE, T_THEME, T_OPEN, T_BACK,
    T_NO_IMAGE, T_FAILED,
    T_ACTIONS, T_RENAME, T_MOVE, T_OPEN_SYSTEM, T_DELETE,
    T_PREV_FILE, T_NEXT_FILE, T_FIRST, T_PREV_PAGE, T_NEXT_PAGE, T_LAST,
    T_PRINT, T_PRINT_PAGE,
    T_NEW_NAME, T_DEST_DIR, T_CANCEL,
    T_RENAMED, T_MOVED, T_COULD_NOT_RENAME, T_COULD_NOT_MOVE,
    T_SETTINGS, T_HELP, T_WINDOW, T_SORT, T_SEARCH,
    T_DELETE_FILE, T_DELETE_CONFIRM, T_CLOSE, T_SLIDESHOW,
    T_PREV, T_NEXT, T_STOP, T_PLAY, T_PAUSE, T_SHUFFLE, T_RECURSIVE,
    T_NO_IMAGES, T_COULD_NOT_READ_DIR, T_FILE_DELETED, T_COULD_NOT_DELETE,
    T_OPEN_FOLDER, T_UPDATE_THUMBS, T_CHECKERBOARD, T_SHOW_PREVIEW,
    T_START_MAXIMIZED,
    T_COUNT
};

void i18n_set_language(int lang);
const char *i18n_msg(int id);

#define msg(id) i18n_msg(id)

/* Each language file defines one of these. */
extern const char *const lang_en[T_COUNT];
extern const char *const lang_de[T_COUNT];
extern const char *const lang_es[T_COUNT];
extern const char *const lang_fr[T_COUNT];
extern const char *const lang_it[T_COUNT];
extern const char *const lang_pt[T_COUNT];

#endif /* I18N_H */