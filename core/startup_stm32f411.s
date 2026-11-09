.syntax unified
.cpu cortex-m4
.thumb

.global _estack
.global Reset_Handler

/* Vector table */
.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object
.extern SVC_Handler
.extern PendSV_Handler
.extern SysTick_Handler
<<<<<<< Updated upstream
=======
.extern TIM2_IRQHandler
.extern EXTI0_IRQHandler
.extern EXTI1_IRQHandler
.extern EXTI2_IRQHandler
.extern EXTI3_IRQHandler
.extern EXTI4_IRQHandler
.extern EXTI9_5_IRQHandler
.extern EXTI15_10_IRQHandler
.extern USART2_IRQHandler
.extern DMA1_Stream6_IRQHandler
>>>>>>> Stashed changes

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

    .word SVC_Handler /* SVC */
    .word Default_Handler /* DebugMon */
    .word 0
    .word PendSV_Handler /* PendSV */
    .word SysTick_Handler /* SysTick */

<<<<<<< Updated upstream
    /* IRQs (rút gọn, sau này add thêm) */
    .rept 82
=======
    /* External IRQs: EXTI0..4=6..10, EXTI9_5=23, TIM2=28, EXTI15_10=40. */
    .rept 6
    .word Default_Handler
    .endr
    .word EXTI0_IRQHandler
    .word EXTI1_IRQHandler
    .word EXTI2_IRQHandler
    .word EXTI3_IRQHandler
    .word EXTI4_IRQHandler
    .rept 6
    .word Default_Handler
    .endr
    .word DMA1_Stream6_IRQHandler /* IRQ17 */
    .rept 5
    .word Default_Handler
    .endr
    .word EXTI9_5_IRQHandler
    .rept 4
    .word Default_Handler
    .endr
    .word TIM2_IRQHandler
    .rept 9
    .word Default_Handler
    .endr
    .word USART2_IRQHandler /* IRQ38 */
    .word Default_Handler
    .word EXTI15_10_IRQHandler
    .rept 41
>>>>>>> Stashed changes
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

    bl system_init
/* Call main */
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