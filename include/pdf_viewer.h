#ifndef PDF_VIEWER_H
#define PDF_VIEWER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle to a PDF viewer instance. */
typedef struct pdf_viewer pdf_viewer;

/* Lifecycle. `ui` may be NULL for create/destroy of the handle itself, but
 * is required for open/close/render operations. */
pdf_viewer *pdf_viewer_create(void);
void pdf_viewer_destroy(pdf_viewer *v, void *ui);

/* Open a PDF document and render its first page. Returns true on success. */
bool pdf_viewer_open(pdf_viewer *v, void *ui, const char *path);

/* Close the document and free all resources. Safe to call repeatedly. */
void pdf_viewer_close(pdf_viewer *v, void *ui);

/* Navigation. Returns true on success. */
bool pdf_viewer_first(pdf_viewer *v, void *ui);
bool pdf_viewer_last(pdf_viewer *v, void *ui);
bool pdf_viewer_prev(pdf_viewer *v, void *ui);
bool pdf_viewer_next(pdf_viewer *v, void *ui);
bool pdf_viewer_goto(pdf_viewer *v, void *ui, int page);

/* State queries. */
bool pdf_viewer_is_open(const pdf_viewer *v);
const char *pdf_viewer_path(const pdf_viewer *v);
int pdf_viewer_page(const pdf_viewer *v);       /* 0-based */
int pdf_viewer_page_count(const pdf_viewer *v);
void *pdf_viewer_texture(const pdf_viewer *v);  /* cui_texture as void* */
int pdf_viewer_width(const pdf_viewer *v);
int pdf_viewer_height(const pdf_viewer *v);

/* Access to the zoom/pan state stored in the viewer. */
void *pdf_viewer_view(pdf_viewer *v);            /* cui_image_view* */

/* Printing (best effort). Renders the given 1-based inclusive page range to
 * temporary PNG files and submits them to the system printer. Returns true
 * if the submission command was launched successfully. */
bool pdf_viewer_print_range(pdf_viewer *v, void *ui, int first_page, int last_page);

#ifdef __cplusplus
}
#endif

#endif /* PDF_VIEWER_H */