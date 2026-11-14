#include "settings.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char *p = settings_default_path();
    assert(p != NULL);
    assert(strstr(p, "settings.json") != NULL);
    free(p);

    /* Relative path keeps the test portable (no POSIX mkdir needed). */
    char tmp[] = "cuiviewer_test_settings.json";

    app_settings s;
    settings_defaults(&s);
    s.lang = 2;
    s.dark = true;
    snprintf(s.last_directory, sizeof s.last_directory, "/tmp/pics");
    s.slideshow_interval_seconds = 3.5;
    s.thumbnails_size = 64;
    s.custom_command_count = 1;
    snprintf(s.custom_commands[0].name, sizeof s.custom_commands[0].name, "proc");
    snprintf(s.custom_commands[0].command, sizeof s.custom_commands[0].command, "proc %s", "/tmp/f");
    snprintf(s.custom_commands[0].shell, sizeof s.custom_commands[0].shell, "/bin/sh");
    s.custom_commands[0].terminal = true;
    assert(settings_save(&s, tmp) == 0);

    app_settings s2;
    settings_defaults(&s2);
    assert(settings_load(&s2, tmp) == 0);
    assert(s2.lang == 2);
    assert(s2.dark == true);
    assert(strcmp(s2.last_directory, "/tmp/pics") == 0);
    assert(s2.slideshow_interval_seconds == 3.5);
    assert(s2.thumbnails_size == 64);
    assert(s2.custom_command_count == 1);
    assert(strcmp(s2.custom_commands[0].name, "proc") == 0);
    assert(s2.custom_commands[0].terminal == true);

    /* missing file keeps defaults */
    app_settings s3;
    settings_defaults(&s3);
    remove("cuiviewer_test_missing.json");
    assert(settings_load(&s3, "cuiviewer_test_missing.json") == 0);
    assert(s3.lang == 0);

    remove(tmp);
    printf("settings tests passed\n");
    return 0;
}