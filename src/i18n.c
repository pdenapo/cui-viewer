#include "i18n.h"

#include <stddef.h>

static int current_lang = LANG_EN;

void i18n_set_language(int lang) {
    if (lang < 0 || lang >= LANG_COUNT) lang = LANG_EN;
    current_lang = lang;
}

const char *i18n_msg(int id) {
    if (id < 0 || id >= T_COUNT) return "";
    const char *const *table = lang_en;
    switch (current_lang) {
        case LANG_DE: table = lang_de; break;
        case LANG_ES: table = lang_es; break;
        case LANG_FR: table = lang_fr; break;
        case LANG_IT: table = lang_it; break;
        case LANG_PT: table = lang_pt; break;
        default: table = lang_en; break;
    }
    return table[id] ? table[id] : "";
}