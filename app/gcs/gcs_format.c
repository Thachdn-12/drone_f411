#include "gcs_format.h"
#include <stdarg.h>
#include <stdint.h>
#include <math.h>

typedef struct { char *data; size_t used, capacity; int failed; } writer_t;

static void put(writer_t *w, char c)
{
    if (w->used + 1U >= w->capacity) { w->failed = 1; return; }
    w->data[w->used++] = c;
}

static void string(writer_t *w, const char *s)
{
    while (*s) put(w, *s++);
}

static void integer(writer_t *w, uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do { digits[count++] = (char)('0' + value % 10U); value /= 10U; } while (value);
    while (count) put(w, digits[--count]);
}

static void decimal(writer_t *w, double value)
{
    if (!isfinite(value)) { string(w, "null"); return; }
    /* Telemetry is bounded well below this; guard the integer conversion. */
    if (value > 1000000.0 || value < -1000000.0) { w->failed = 1; return; }
    if (value < 0) { put(w, '-'); value = -value; }
    uint32_t scaled = (uint32_t)(value * 100.0 + 0.5);
    integer(w, scaled / 100U);
    put(w, '.');
    put(w, (char)('0' + (scaled / 10U) % 10U));
    put(w, (char)('0' + scaled % 10U));
}

int gcs_snprintf(char *out, size_t capacity, const char *format, ...)
{
    if (!out || !capacity || !format) return -1;
    writer_t w = {out, 0, capacity, 0};
    va_list args;
    va_start(args, format);
    while (*format && !w.failed)
    {
        if (*format != '%') { put(&w, *format++); continue; }
        ++format;
        if (*format == 's') { string(&w, va_arg(args, const char *)); ++format; }
        else if (*format == 'u') { integer(&w, va_arg(args, unsigned int)); ++format; }
        else if (format[0] == 'l' && format[1] == 'u')
        { integer(&w, (uint32_t)va_arg(args, unsigned long)); format += 2; }
        else if (format[0] == '.' && format[1] == '2' && format[2] == 'f')
        { decimal(&w, va_arg(args, double)); format += 3; }
        else if (*format == '%') { put(&w, '%'); ++format; }
        else w.failed = 1;
    }
    va_end(args);
    out[w.used] = '\0';
    return w.failed ? -1 : (int)w.used;
}
