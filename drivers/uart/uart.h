#ifndef UART_H
#define UART_H

#include <stdint.h>

/* USART2 PA2 TX / PA3 RX, AF7, 115200 8N1 at APB1=16 MHz.
 * RX uses USART2 IRQ; TX owns DMA1 Stream6 Channel4 and copies caller data.
 * One foreground producer/consumer; do not call from ISRs or multiple tasks.
 * Buffer sizes include one reserved ring slot. No RTOS dependency.
 */
#define UART_RX_BUFFER_SIZE 256U
#define UART_TX_BUFFER_SIZE 1024U
#define UART_PRINTF_SIZE    256U
#define UART_DEFAULT_TIMEOUT_MS 20U

typedef enum
{
    UART_OK = 0,
    UART_ERR_ARGUMENT = -1,
    UART_ERR_NOT_READY = -2,
    UART_ERR_WOULD_BLOCK = -3,
    UART_ERR_TIMEOUT = -4,
    UART_ERR_IO = -5,
    UART_ERR_TOO_LONG = -6
} uart_status_t;

typedef struct
{
    uint32_t rx_dropped;
    uint32_t rx_errors;
    uint32_t rx_overruns;
    uint32_t tx_rejected_writes;
    uint32_t tx_dma_errors;
} uart_stats_t;

/* Bounded initialization, usable before SysTick. Reinitialization discards
 * queued data and resets counters; it also recovers a latched DMA fault.
 * Preserves PRIMASK and other DMA streams. */
uart_status_t uart_init(void);

/* Nonblocking, all-or-nothing enqueue. len=1..1023; caller buffer can be reused
 * immediately. Full queue returns WOULD_BLOCK. DMA faults latch IO until init.
 * OK means queued, not yet physically transmitted. */
uart_status_t uart_write(const uint8_t *data, uint32_t len);

/* Nonblocking dequeue, up to capacity bytes (including binary NUL). On a
 * valid received pointer, always sets *received, even on errors. Empty RX
 * returns WOULD_BLOCK. Overflow drops newest bytes, preserving unread data. */
uart_status_t uart_read(uint8_t *data, uint32_t capacity, uint32_t *received);
uint32_t uart_rx_available(void);
void uart_get_stats(uart_stats_t *stats);

/* Foreground bounded waits, using SysTick with finite polling fallback before
 * SysTick starts/if it stops. timeout=0 tests once. Flush waits for UART TC,
 * including the final stop bit. Timeout leaves queues/transmission intact. */
uart_status_t uart_read_timeout(uint8_t *byte, uint32_t timeout_ms);
uart_status_t uart_flush(uint32_t timeout_ms);

/* Bounded formatter: %c %s %d %u %x %% (hex keeps 0x + eight digits).
 * Up to UART_PRINTF_SIZE bytes, no partial/truncated message is queued. */
uart_status_t uart_printf_try(const char *fmt, ...);

/* Legacy wrappers. Writes/printf now enqueue without waiting; queue-full,
 * size and DMA-fault rejections are counted (invalid arguments are not).
 * Prefer status-returning APIs where delivery matters.
 * read_char returns NUL on failure after a 20 ms bound. */
void uart_write_char(char c);
void uart_write_string(const char *str);
char uart_read_char(void);
void uart_printf(const char *fmt, ...);

#endif
