.syntax unified
.cpu cortex-m4
.thumb

.global _estack
.global Reset_Handler

/* Vector table */
.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object

g_pfnVectors:
    .word _estack
    .word Reset_Handler

    /* Exceptions */
    .word Default_Handler /* NMI */
    .word Default_Handler /* HardFault */
    .word Default_Handler /* MemManage */
    .word Default_Handler /* BusFault */
    .word Default_Handler /* UsageFault */

    .word 0
    .word 0
    .word 0
    .word 0

    .word Default_Handler /* SVC */
    .word Default_Handler /* DebugMon */
    .word 0
    .word Default_Handler /* PendSV */
    .word Default_Handler /* SysTick */

    /* IRQs (rút gọn, sau này add thêm) */
    .rept 82
    .word Default_Handler
    .endr

.size g_pfnVectors, .-g_pfnVectors

/* Reset Handler */
.section .text.Reset_Handler
.type Reset_Handler, %function

Reset_Handler:
    /* Copy .data */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_etext

copy_data:
    cmp r0, r1
    bcc copy_word
    b zero_bss

copy_word:
    ldr r3, [r2], #4
    str r3, [r0], #4
    b copy_data

zero_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss

zero_loop:
    cmp r0, r1
    bcc zero_word
    b main

zero_word:
    movs r2, #0
    str r2, [r0], #4
    b zero_loop

/* Call main */
main:
    bl main

loop:
    b loop

.size Reset_Handler, .-Reset_Handler

/* Default handler */
.section .text.Default_Handler
.type Default_Handler, %function

Default_Handler:
    b .

.size Default_Handler, .-Default_Handler