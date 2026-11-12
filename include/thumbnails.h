#ifndef THUMBNAILS_H
#define THUMBNAILS_H

#include <stdbool.h>
#include <stddef.h>

/* Resolve the thumbnail file path for a source image according to the
 * configured storage location. Returns false when thumbnails are disabled
 * ("none") or the path cannot be built. */
bool thumbnail_path_for(const char *source, const char *location, char *out, size_t cap);

/* Ensure a thumbnail exists and is up to date for the given source image.
 * Returns true on success and writes the thumbnail path to out. */
bool thumbnail_ensure(const char *source, int size, const char *location, char *out, size_t cap);

/* Remove a directory's worth of thumbnails? Reserved for future use. */
void thumbnail_cache_init(void);

#endif