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
- Use `uart_printf("Hello UART\r\n");` after `uart_init()`.
- The baud divider assumes the reset HSI clock with APB1 at 16 MHz.
  Recalculate it if the clock configuration changes.
- PA2 and PA3 are reserved for UART; `adc_init()` configures PA0 and PA1 only.

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

Boot logs are sent before SysTick and sensor initialization. The current
application uses a bare-metal 1 ms SysTick at 16 MHz, without starting the
FreeRTOS scheduler. I2C waits time out after 20 ms per stage; if MPU6050 fails
initialization, the console prints `UART OK; MPU6050 unavailable` every second.
BMP280 initialization and periodic logging also work in this fallback loop.

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
reset the board. A working sensor prints `BMP280: ready`, then once per second:

```text
BMP280: T_centiC=2508 P_Pa=100653
```

Here `2508` means 25.08 degrees C and `100653` means 100653 Pa (1006.53 hPa).
Values are measured pressure and temperature; altitude estimation and altitude
control are not implemented. Initialization errors include `-2` (SPI I/O),
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
short and use flags/counters for work in the main loop. Do not call blocking
UART, sensor or delay APIs from callbacks. Attach from main/thread context and
keep context data valid until detached. There is no debounce or edge queue;
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
