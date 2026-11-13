#include "settings.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    char *p = settings_default_path();
    assert(p != NULL);
    assert(strstr(p, "settings.json") != NULL);
    free(p);

    char tmp[] = "/tmp/opencode/test_settings.json";
    /* ensure dir */
    char dir[] = "/tmp/opencode";
    mkdir(dir, 0755);

    app_settings s;
    settings_defaults(&s);
    s.lang = 2;
    s.dark = true;
    snprintf(s.last_directory, sizeof s.last_directory, "/tmp/pics");
    s.slideshow_interval_seconds = 3.5;
    s.thumbnails_size = 64;
    assert(settings_save(&s, tmp) == 0);

    app_settings s2;
    settings_defaults(&s2);
    assert(settings_load(&s2, tmp) == 0);
    assert(s2.lang == 2);
    assert(s2.dark == true);
    assert(strcmp(s2.last_directory, "/tmp/pics") == 0);
    assert(s2.slideshow_interval_seconds == 3.5);
    assert(s2.thumbnails_size == 64);

    /* missing file keeps defaults */
    app_settings s3;
    settings_defaults(&s3);
    assert(settings_load(&s3, "/tmp/opencode/nope.json") == 0);
    assert(s3.lang == 0);

    remove(tmp);
    printf("settings tests passed\n");
    return 0;
}