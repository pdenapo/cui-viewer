#include "thumbnails.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    char out[4096];

    /* none -> false */
    assert(!thumbnail_path_for("/tmp/a.png", "none", out, sizeof out));

    /* per-directory */
    assert(thumbnail_path_for("/tmp/dir/img.jpg", "per-directory", out, sizeof out));
    assert(strstr(out, "/tmp/dir/.xvpics/img.png") != NULL);

    /* global non-empty */
    assert(thumbnail_path_for("/tmp/dir/img.jpg", "global", out, sizeof out));
    assert(strlen(out) > 0);
    assert(strstr(out, "thumbnails") != NULL);

    /* null args */
    assert(!thumbnail_path_for(NULL, "per-directory", out, sizeof out));
    assert(!thumbnail_path_for("/tmp/a.png", NULL, out, sizeof out));

    printf("thumbnail path tests passed\n");
    return 0;
}