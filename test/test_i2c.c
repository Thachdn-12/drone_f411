#include <stdint.h>
#include "i2c.h"

int main(void)
{
    i2c_init();

    uint8_t id = 0;

    while (1)
    {
        i2c_read(0x68, 0x75, &id, 1);

        // đặt breakpoint tại đây
        for (volatile int i = 0; i < 1000000; i++);
    }
}