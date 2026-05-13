#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "uart.h"

extern uint8_t _end;
static uint8_t *heap_end;

int _write_r(struct _reent *r, int file, const void *ptr, size_t len)
{
    (void)r;
    (void)file;

    const char *c = (const char *)ptr;

    for (size_t i = 0; i < len; i++) {

        if (c[i] == '\n')
            uart_write_char('\r');

        uart_write_char(c[i]);
    }

    return len;
}

int fputc(int ch, FILE *f)
{
    uart_write_char((char)ch);
    return ch;
}

int _close(int file)
{
    return -1;
}

int _fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    return 0;
}

int _read(int file, char *ptr, int len)
{
    return 0;
}

void *_sbrk(ptrdiff_t incr)
{
    return (void *)-1;
}

void _exit(int status)
{
    while (1);
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    return -1;
}

int _getpid(void)
{
    return 1;
}