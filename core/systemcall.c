#include <unistd.h>
#include "uart.h"

int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        uart_write_char(ptr[i]);
    }
    return len;
}