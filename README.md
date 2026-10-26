# Drone Firmware Project

## Overview
Custom quadcopter flight controller firmware using:

- STM32F411 Black Pill
- MPU6050 IMU
- BLHeli_S ESC
- ESP32 WiFi module

The project follows a hybrid realtime + event-driven architecture for building a modular and scalable drone firmware system.

---

## Main Features

- MPU6050 sensor reading
- PID stabilization loop
- ESC PWM control
- ESP32 WiFi communication
- Event queue + dispatcher
- Realtime scheduling

---

## Hardware Stack

- STM32F411
- MPU6050
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
