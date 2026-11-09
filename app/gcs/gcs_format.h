#ifndef GCS_FORMAT_H
#define GCS_FORMAT_H
#include <stddef.h>

/* Allocation-free subset used by gcs_link: %s, %u, %lu, %.2f and %%.
 * Non-finite numbers become JSON null. Returns -1 on overflow/invalid format.
 */
int gcs_snprintf(char *out, size_t capacity, const char *format, ...);
#endif
