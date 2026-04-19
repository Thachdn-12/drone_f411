#include <stdint.h>
#include <stdio.h>

#define SYST_CSR   (*(volatile uint32_t*)0xE000E010)
#define SYST_RVR   (*(volatile uint32_t*)0xE000E014)
#define SYST_CVR   (*(volatile uint32_t*)0xE000E018)

volatile uint32_t tick = 0;

void systick_init(void)
{
    /* 16MHz / 1000 = 16000 → 1ms */
    SYST_RVR = 16000 - 1;

    SYST_CVR = 0;

    /* enable + interrupt + processor clock */
    SYST_CSR = (1 << 0) | (1 << 1) | (1 << 2);
}

void SysTick_Handler(void)
{
    tick++;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = tick;
    while ((tick - start) < ms){};
}