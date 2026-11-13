#include "i18n.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    for (int lang = 0; lang < LANG_COUNT; lang++) {
        i18n_set_language(lang);
        for (int id = 0; id < T_COUNT; id++) {
            const char *s = i18n_msg(id);
            assert(s != NULL);
            assert(s[0] != '\0');
        }
    }
    /* switching changes output */
    i18n_set_language(LANG_EN);
    const char *en = i18n_msg(T_SETTINGS);
    i18n_set_language(LANG_ES);
    const char *es = i18n_msg(T_SETTINGS);
    assert(strcmp(en, es) != 0);
    printf("i18n tests passed\n");
    return 0;
}