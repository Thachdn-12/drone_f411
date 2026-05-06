#include "uart.h"
#include <stdio.h>

int main(){ 
    uart_init(115200);
    int x = 42;
    while (1) {
        uart_write_string("HELLO\r\n");
        printf("test: %d", x);
        for (volatile int i = 0; i < 1000000; i++);
    }
    return 0;
}
