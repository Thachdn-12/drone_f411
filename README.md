# Drone Firmware Project

## Overview
Custom quadcopter flight controller firmware using:

- STM32F411 Black Pill
- MPU6050 IMU
- BMP280 pressure/temperature sensor (SPI1)
- BLHeli_S ESC
- ESP32 WiFi module

The project follows a hybrid realtime + event-driven architecture for building a modular and scalable drone firmware system.

---

## Main Features

- MPU6050 sensor reading
- BMP280 compensated pressure and temperature over 4-wire SPI
- PID stabilization loop
- ESC PWM control
- ESP32 WiFi communication
- Event queue + dispatcher
- Realtime scheduling

---

## Hardware Stack

- STM32F411
- MPU6050
- BMP280 module with exposed CSB and SDO pins
- BLHeli_S 30A ESC
- 2205 2300KV Brushless Motors
- 3S LiPo Battery
- UBEC 5V

---

## Firmware Architecture

```text
Application Layer
    ↓
Service Layer
    ↓
Driver Layer
    ↓
Hardware
```

---

## Project Structure

```text
/drone_fw
├── core/
├── kernel/
├── drivers/
├── app/
```

---

## Development Goals

- Learn realtime embedded systems
- Build event-driven firmware
- Develop flight control logic
- Improve firmware architecture skills

---

## Status

- Hardware selected
- Architecture planning completed
- Kernel development started

## Build

The default firmware includes the GCS v2 UART link and bench motor hooks.
See [GCS integration](docs/GCS_INTEGRATION.md) for wiring, the app update and
checks. PB0 to GND enables bench tests; motor outputs stay off during boot.
Periodic console logs are suppressed in GCS mode.

With `arm-none-eabi-gcc` on PATH, run `make -j4`. A toolchain installed at
`toolchains/bin` is detected automatically. To use another
installation:

```sh
make -j4 TOOLCHAIN_DIR=/path/to/toolchain/bin
```

Build artifacts and header dependencies are stored under `output/`, including
`drone.elf`, `drone.bin`, and `drone.map`. Use `make flash` to flash with ST-Link;
pass the same `TOOLCHAIN_DIR` override if needed.

The Makefile provides CMSIS Core, CMSIS Driver, STM32F411 device, and CMSIS-DSP
include paths, with `STM32F411xE` selected. Application code can include
`stm32f4xx.h`. The current `libs/CMSIS/Driver` directory contains only headers;
there are no driver C sources to compile. Add individual DSP source files to
`CMSIS_DSP_SRC` in the Makefile when using additional DSP functions.

## Flash and Debug on Ubuntu (ST-Link)

The Makefile uses the bundled OpenOCD in `tools/xpack-openocd-0.12.0-7`, including
its scripts. Connect ST-Link SWDIO to PA13, SWCLK to PA14, and GND to GND; power
the board and connect the probe's target voltage reference if required by the
probe. SWD uses a software reset, so NRST is optional.

```sh
make flash                  # Build, program, verify, reset and run
make flash SWD_SPEED=400    # Lower SWD speed if the connection is unstable
```

For debugging, start the OpenOCD server in one terminal, then GDB in another:

```sh
make debug
```

```sh
make gdb
```

GDB connects to localhost:3333, resets and halts the CPU, then runs to `main`.
Use `next`, `step`, `print variable`, and `continue` in GDB. Flash new firmware
with `make flash` before starting the debug server.

Override the OpenOCD installation with `OPENOCD_DIR=/path/to/xpack-openocd`, or
set `OPENOCD=/path/to/openocd OPENOCD_SCRIPTS=/path/to/scripts`. Override the
debugger with `GDB=gdb-multiarch` if the bundled GDB needs unavailable libraries.

If Ubuntu reports USB permission errors, install the supplied udev rules once:

```sh
sudo install -m 644 tools/xpack-openocd-0.12.0-7/openocd/contrib/60-openocd.rules /etc/udev/rules.d/60-openocd.rules
sudo udevadm control --reload-rules
```

Unplug and reconnect ST-Link after installing the rules.

## UART Console

- USART2: PA2 (TX), PA3 (RX), AF7.
- Connect PA2 to the RX pin of a 3.3 V USB-UART adapter and connect GND.
  Connect PA3 to the adapter's TX pin if receiving is needed.
- Open the adapter's serial port at 115200 baud, 8 data bits, no parity,
  1 stop bit, and no flow control; reset the board to see boot logs.
- Check that `uart_init()` returns `UART_OK`, then use
  `uart_printf("Hello UART\r\n");` to enqueue a log.
- The baud divider assumes the reset HSI clock with APB1 at 16 MHz.
  Recalculate it if the clock configuration changes.
- PA2 and PA3 are reserved for UART; `adc_init()` configures PA0 and PA1 only.

UART now receives through the USART2 interrupt into a 256-byte ring (255 usable
bytes), and transmits from a 1024-byte ring (1023 usable bytes) using DMA1
Stream6 Channel4. DMA transmission copies caller data into the ring before
returning, so stack buffers may be reused immediately. UART initialization and
ordinary reads/writes do not require SysTick. The startup table connects both
USART2 IRQ38 and DMA1 Stream6 IRQ17; RX uses priority 14 and DMA uses priority 15.
The driver exclusively owns USART2 and DMA1 Stream6. Other DMA streams, GPIO
pins, TIM2 and SysTick retain their existing configuration. DMA routing follows
ST's [RM0383](https://www.st.com/resource/en/reference_manual/DM00119316.pdf)
and [AN4616](https://www.st.com/resource/en/application_note/an4616-migrating-from-stm32f401-and-stm32f411-lines-to-stm32l4-series-and-stm32l4-series-microcontrollers-stmicroelectronics.pdf).

Use the status-returning APIs for protocol data:

```c
uint8_t input[32];
uint32_t received;
uart_status_t status = uart_read(input, sizeof input, &received);
if (status == UART_OK) {
    /* Process exactly 'received' bytes; binary NUL is valid data. */
}

const uint8_t message[] = {'O', 'K', '\r', '\n'};
status = uart_write(message, sizeof message);
/* UART_OK means queued. Retry later on UART_ERR_WOULD_BLOCK. */
```

`uart_write()` enqueues the entire message or none of it; queue-full returns
`UART_ERR_WOULD_BLOCK`, and requests larger than 1023 bytes return
`UART_ERR_TOO_LONG`. An empty RX ring also returns `UART_ERR_WOULD_BLOCK`, with
`received=0`. RX overflow drops the newest byte and preserves unread data.
Parity/framing/noise/overrun errors are acknowledged by the SR/DR read sequence;
bytes flagged with errors are discarded. `uart_get_stats()` reports RX drops,
error events/overruns, TX queue-full/size/fault rejections and DMA errors. Counters wrap modulo
2^32 and reset on reinitialization.

`uart_printf_try()` returns the enqueue/formatting status. Formatting supports
`%c`, `%s`, `%d`, `%u`, `%x` and `%%`; `%x` retains the old `0x` plus eight-digit
format. A formatted message may contain up to 256 bytes. Longer messages are
rejected without queueing a truncated prefix. Legacy `uart_printf()`,
`uart_write_string()` and `uart_write_char()` still work, but now enqueue
without waiting and cannot return queue-full errors; use the status APIs when
delivery matters. These are single-foreground-owner APIs: do not call them
from ISRs, callbacks or concurrent RTOS tasks without adding serialization.

`uart_read_timeout(&byte, timeout_ms)` waits for one byte, and
`uart_flush(timeout_ms)` waits until the queue drains and UART TC confirms the
last stop bit has left the wire. These two calls block and belong outside the
timed control path. Both have a SysTick deadline and a finite polling limit;
whichever limit is reached first ends the wait, even before SysTick starts or
if it stops. A zero timeout checks once. Timeout preserves queued data and an
in-flight transmission. Legacy `uart_read_char()` has a 20 ms timeout and
returns NUL on error; use the status API to distinguish errors from valid NUL.

A TX DMA transfer/direct-mode fault latches `UART_ERR_IO`; RX remains active,
and queued bytes are not replayed because part of the message may already
have been transmitted.
In direct mode, a FIFO underrun flag is acknowledged without stopping TX:
RM0383 section 9.3.18 specifies that DMA retains the pending request without
data loss. FIFO error interrupts are disabled, and each transfer arms the DMA
stream before enabling USART TX DMA requests.
Check `uart_init()` to recover from a transfer/direct-mode fault: it waits a
bounded time for the stream to stop, then discards both rings and resets
counters. Normal startup works with DMA
before SysTick, including the boot banner. Global interrupts must be enabled
for asynchronous TX/RX to progress. Host checks are available with
`make test-uart`; verify console output, loopback and timing on the board too.

To view logs on Ubuntu:

```sh
make flash
make monitor PORT=/dev/ttyUSB0
```

Press the board's RESET button after opening the terminal to see the boot log.
Exit screen with Ctrl+A, then K, then Y. Use `ls /dev/serial/by-id/` to identify
the adapter; a full `/dev/serial/by-id/...` path also works as `PORT`.
Close other applications using the serial port first. If access is denied,
add your Ubuntu user to `dialout` with `sudo usermod -aG dialout "$USER"`, then
log out and back in.

Boot logs are queued before SysTick and sensor initialization. The current
application uses a bare-metal 1 ms SysTick for delays/timeouts and TIM2 for
foreground task releases at 16 MHz, without starting the FreeRTOS scheduler.
I2C waits use a 20 ms per-stage deadline plus a finite polling fallback and
bounded bus recovery. If MPU6050 initialization fails, GCS and barometer tasks
keep running; telemetry marks IMU measurements unavailable.

## I2C1 timeout and bus recovery

I2C1 uses PB6 (SCL) and PB7 (SDA), AF4 open-drain, at 100 kHz with the current
16 MHz APB1 clock. Call `i2c_init()` before transactions. Device addresses are
unshifted 7-bit values, such as MPU6050 `0x68`. Use suitable external pull-ups
to 3.3 V on both lines; internal pull-ups are also enabled. This driver assumes
one controller on the bus and is blocking/non-reentrant, for foreground use.

Every wait for a peripheral flag, bus idle/STOP completion or released GPIO
clock/data line has both a 20 ms SysTick deadline and a 100000-poll limit. The
first limit reached ends the wait, so it cannot wait forever before SysTick is
initialized, when interrupts are masked, or if the tick stops. Unsigned elapsed
arithmetic handles SysTick rollover. These are per-stage limits, not a single
20 ms limit for the entire transaction or recovery sequence.

Before a transaction, a stuck BUSY flag, disabled peripheral or low SCL/SDA
triggers one recovery attempt before sending any address or data. Recovery:

1. Disables I2C1 and switches PB6/PB7 to open-drain GPIO with both lines released.
2. Waits a bounded time for SCL to rise, allowing temporary clock stretching.
3. If SDA remains low, sends up to nine SCL recovery pulses, stopping early when
   SDA is released. GPIO writes of one release the line; they do not drive high.
4. Generates a STOP sequence, restores AF4 and resets/reconfigures only I2C1.
5. Checks that the peripheral is idle before reporting success.

Recovery half-period delays use bounded CPU loops and `system_core_clock()`,
without `delay_ms()` or SysTick. A STOP sequence includes its own SCL transition
in addition to the recovery pulses. Other GPIO pins, UART, TIM2 and SysTick are
not reconfigured, and recovery waits leave interrupts in their existing state.

Call `i2c_recover_bus()` explicitly if needed; it returns `0` on success or
`-1` if recovery fails or I2C was not initialized. Both pins are released and
restored to AF4 even on failure; PE remains disabled after a failed clear.
A later transaction can attempt recovery again. A device holding SCL low, a
short, or SDA remaining low after nine clocks cannot be fixed by software
alone; reset/power-cycle the affected device or correct the wiring. The bus-clear
procedure follows [NXP UM10204, section 3.1.16](https://www.nxp.com/docs/en/user-guide/UM10204.pdf).

Transaction APIs retain `0`/`-1` results. On failure, the driver attempts a
bounded STOP/cleanup and recovers the bus if it remains stuck. An arbitration
loss resets only the local peripheral, without issuing STOP or GPIO clocks.
No failed transaction is automatically replayed: a write may already have been
accepted before a timeout. Discard partial data from a failed
`i2c_read_bytes()` call. `i2c_read_reg()` only updates its output after the
complete transaction, including STOP, succeeds.

Receive handling now distinguishes one, two and three-or-more bytes, including
the ACK/POS/BTF/STOP sequencing needed for MPU6050's 14-byte burst. Brief
critical sections preserve PRIMASK around the receive tail; waits and bus-clear
loops stay outside those sections. The sequence is based on the STM32F4
[I2C implementation from ST](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Src/stm32f4xx_hal_i2c.c);
no HAL dependency is added.

Run `make test-i2c` for the host protocol/pin model tests: recovery after zero
through nine pulses, SDA/SCL stuck low, clock stretching, restored pin state,
frozen/wrapping SysTick, transaction failures without replay, receive lengths
1/2/3/4/14/32/255 and MPU6050 raw-data decoding. The model simulates register
side effects; validate electrical timing and recovery on the board with actual
devices and a logic analyzer.

## BMP280 over SPI

The firmware uses SPI1 in 4-wire mode, mode 0, MSB first, 8-bit frames.
APB2/16 gives a 1 MHz clock with the current 16 MHz system clock.
Connect a module with exposed CSB/CS and SDO pins as follows:

| BMP280 module | STM32F411 |
| --- | --- |
| VCC / VIN (and VDDIO if exposed separately) | 3.3 V |
| GND | GND |
| SCK / SCL | PA5 (SPI1 SCK) |
| SDO | PA6 (SPI1 MISO) |
| SDI / SDA | PA7 (SPI1 MOSI) |
| CSB / CS | PA4 (GPIO chip select, active low) |

SCL/SDA labels on a dual-interface module serve as SCK/SDI in SPI mode.
CSB must be controllable by PA4; a four-pin module exposing only VCC, GND,
SCL and SDA cannot use this connection. Use 3.3 V power and logic. See the
[Bosch BMP280 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf),
sections 5.3 and 6.1, for the SPI protocol and pin functions.
PA4..PA7 do not overlap the current MPU6050 I2C, UART, ADC or ESC PWM pins.

Startup checks chip ID `0x58`, resets the sensor, waits for the calibration
copy, and reads all temperature/pressure calibration coefficients. The driver
uses normal mode with temperature oversampling x2, pressure x16, IIR filter x4
and 0.5 ms standby. It reads all six data bytes in one SPI burst and applies
the compensation formulas from section 8.1 of the Bosch datasheet using the
Cortex-M4F single-precision FPU. The first conversion is allowed 50 ms.

After `make flash`, open `make monitor PORT=/dev/ttyUSB0` at 115200 baud and
reset the board. A working sensor prints `BMP280: ready`. With GCS disabled,
it also prints a sample once per second (GCS mode sends JSON telemetry):

```text
BMP280: T_centiC=2508 P_Pa=100653
```

Here `2508` means 25.08 degrees C and `100653` means 100653 Pa (1006.53 hPa).
Pressure and temperature are measured. GCS also estimates relative barometric
altitude from the first valid pressure after boot; altitude control is not
implemented. The updated app can derive height from pressure with older firmware. Initialization errors include `-2` (SPI I/O),
`-3` (wrong/missing chip ID), `-4` (invalid calibration), and `-7` (calibration
copy timeout). A missing BMP280 does not prevent MPU6050 operation. Reset the
board after correcting wiring to retry initialization. SPI waits time out after
20 ms per stage, with a finite polling fallback if SysTick stops; initialization
delays require a running SysTick. The APIs are blocking and non-reentrant.

Run `make test-bmp280 test-spi` for host protocol, compensation and SPI timeout
checks. Hardware wiring and signal timing still need verification on the board.

## ADC

ADC1 supports PA0 (`ADC_CHANNEL_0`) and PA1 (`ADC_CHANNEL_1`); PA2/PA3 are reserved
for UART. It uses single software-triggered, right-aligned 12-bit conversions,
480-cycle sampling, and APB2/4 ADC clock. Power-up stabilization and status flag
handling follow ST's [RM0383 reference manual](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf).

Call `systick_init(system_core_clock())` and `adc_init()` before reading:

```c
uint16_t raw;
adc_status_t status = adc_read(ADC_CHANNEL_0, &raw);
if (status == ADC_OK) {
    uart_printf("ADC0=%u\r\n", (unsigned int)raw);
}
```

The API now returns a status and writes the sample through an output pointer.
Only `ADC_OK` updates the output; a valid zero reading is not an error. Invalid
channels/NULL return `ADC_ERR_ARGUMENT`, and a disabled ADC returns
`ADC_ERR_NOT_READY`. Reads time out after 10 ms with a finite polling fallback
if SysTick stops. Timeout or overrun disables the ADC; call `adc_init()` to
recover. The polling API is non-reentrant. Run `make test-adc` for host tests.

## GPIO and external interrupts (EXTI)

The GPIO APIs use pin numbers `0..15`, not bit masks, and support the existing
GPIOA..GPIOE instances. Select pins exposed by your board/package. Enable the
port clock with `gpio_clock_enable()` before ordinary GPIO access. Invalid
ports, pins, enum values and AF numbers are ignored; invalid reads return zero.
Writes and toggles use BSRR so they update only the selected output. Serialize
configuration changes to the same port and concurrent toggles of the same pin.

`gpio_exti_attach()` enables the port and SYSCFG clocks, configures an input
and pull resistor, selects rising/falling/both edges, clears the line's pending
flag and enables its NVIC interrupt. For example, a switch between unused PB0
and GND can be registered from `main()` with:

```c
#include "gpio.h"

static volatile uint32_t input_edges;

static void input_edge(gpio_t *gpio, uint8_t pin, void *context)
{
    (void)gpio;
    (void)pin;
    (void)context;
    ++input_edges;
}

gpio_exti_status_t input_init(void)
{
    return gpio_exti_attach(GPIOB, 0, GPIO_PULL_UP, GPIO_EXTI_FALLING,
                            input_edge, 0);
}
```

Check the returned status before relying on the input. Other trigger choices
are `GPIO_EXTI_RISING` and `GPIO_EXTI_BOTH`. The driver owns GPIO EXTI lines
0..15: PA0 and PB0 share line 0 and cannot both be attached. A conflicting port
returns `GPIO_EXTI_ERR_BUSY`; invalid arguments return `GPIO_EXTI_ERR_ARGUMENT`
without changing hardware. Reattaching the same port/pin replaces its callback,
pull and trigger configuration and discards any old pending flag for that line.

Callbacks receive the originating port, pin and optional context pointer. They
run in interrupt context at fixed NVIC priority 15 (lowest urgency); keep them
short and use flags/counters for work in the main loop. Do not call UART
logging, blocking sensor or delay APIs from callbacks. Attach from main/thread
context and keep context data valid until detached. There is no debounce or edge queue;
multiple edges while a line is already pending may coalesce.

`gpio_exti_detach(GPIOB, 0)` masks and clears only that line, releases its
callback, and leaves the GPIO input/pull configuration unchanged. It returns
`GPIO_EXTI_ERR_NOT_ATTACHED` for a non-owner. A callback may detach itself or a
peer. Shared EXTI9_5 / EXTI15_10 IRQs remain enabled until their last unmasked
line is removed. Attach/detach preserve the caller's interrupt mask.

The startup vector table connects all seven EXTI handlers. Routing, edge
selection and pending-flag acknowledgement follow ST's
[STM32F4 GPIO implementation](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Src/stm32f4xx_hal_gpio.c)
and the bundled STM32F411 device definitions. No HAL dependency is added.

Run `make test-gpio` for host register tests with undefined-behavior checks and
verification of the actual linked interrupt vectors. Host mapped registers do
not emulate electrical edges or W1C hardware; the tests verify the exact
pending-clear write masks. Verify edge detection and switch debounce on board.

## TIM2 task dispatch

TIM2 now drives the foreground application cadence. `main()` checks
`timer_init(DRONE_TIMER_HZ)` before continuing sensor initialization. The default
1 kHz update interrupt only acknowledges the interrupt and increments a tick;
sensor access, angle calculation and UART output run in the main loop.

`app/task_list.c` schedules the following work using the periods in
`config/drone_config.h`:

| Work | Period at the default 1 kHz TIM2 clock | Entry point |
| --- | --- | --- |
| IMU sample and roll/pitch update | 10 ticks / 10 ms (100 Hz target) | `task_imu_run()` |
| Roll/pitch UART log (GCS disabled) | 100 ticks / 100 ms (10 Hz target) | `uart_printf()` |
| BMP280 sample and log | 1000 ticks / 1 s (1 Hz target) | `task_baro_sample()` |

`task_list_init()` synchronizes the phases after the boot logs and sensor
initialization. First releases occur one period later. `task_list_run()` polls
TIM2 and runs each due task at most once per call, with IMU first, then BMP280,
then the optional console angle log. It reads the tick again between tasks to account for time
spent doing I/O. The application no longer uses `delay_ms(100)` to pace this
loop. `task_imu_run()` performs exactly one sample per invocation; its former
independent SysTick timing gate has been removed.

`timer_periodic_start()` and `timer_periodic_poll()` provide reusable periodic
release tracking in TIM2 tick units. For a 10-tick period, a poll at tick 35
after starting at zero returns 3, advances the release phase to tick 30, and
leaves the next release at tick 40. The dispatcher runs the task once and skips
older releases; it does not replay missed sensor samples. Unsigned elapsed
arithmetic handles tick rollover provided fewer than 2^32 ticks have elapsed
since the last release (about 49.7 days at 1 kHz). Poll regularly and restart
the schedules if TIM2 is reinitialized.

The legacy `task_baro_run()` SysTick-paced wrapper remains available, while
the application uses `task_baro_sample()` through TIM2 even if MPU6050 boot
initialization fails. SysTick supplies startup delays, driver timeouts and
GCS timing. The application does not start FreeRTOS.

This is a cooperative dispatcher, with target rates rather than hard deadline
guarantees. Blocking sensor transfers and error timeouts can delay the next IMU
release. UART formatting/enqueueing costs CPU time, but wire transmission now
runs through DMA. TIM2 counts delivered update interrupts; long interrupt
masking can coalesce hardware updates. Measure runtime and jitter on the board
before using this cadence for flight control. PID and motor mixing remain
unimplemented.

Run `make test-scheduler test-timer test-bmp280` for host dispatch/driver tests
and the linked TIM2 vector check. The scheduler tests exercise real TIM2 IRQ
tick increments with fake sensor/UART work, including release boundaries,
simultaneous tasks, skipped periods, delayed work and uint32_t rollover.
