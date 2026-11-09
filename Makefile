# =========================================================
# Project
# =========================================================

PROJECT := drone

# =========================================================
# Build Output
# =========================================================

BUILD_DIR := output

ELF := $(BUILD_DIR)/$(PROJECT).elf
BIN := $(BUILD_DIR)/$(PROJECT).bin
MAP := $(BUILD_DIR)/$(PROJECT).map

# =========================================================
# Toolchain
# =========================================================

PREFIX := arm-none-eabi-

CC      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size
GDB     := $(PREFIX)gdb

# =========================================================
# CPU / MCU
# STM32F411 = Cortex-M4F
# =========================================================

CPUFLAGS := \
	-mcpu=cortex-m4 \
	-mthumb \
	-mfpu=fpv4-sp-d16 \
	-mfloat-abi=hard

# =========================================================
# FreeRTOS Source Files
# STM32F411 = Cortex-M4F
# =========================================================
FREERTOS_SRC := \
	kernel/FreeRTOS/tasks.c \
	kernel/FreeRTOS/queue.c \
	kernel/FreeRTOS/list.c \
	kernel/FreeRTOS/portable/GCC/ARM_CM4F/port.c \
	kernel/FreeRTOS/portable/MemMang/heap_4.c
# =========================================================
# Compiler Flags
# =========================================================

CFLAGS := $(CPUFLAGS)

CFLAGS += \
	-std=c11 \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wundef \
	-O0 \
	-g3 \
	-DSTM32F411xE \
	-DARM_MATH_CM4 \
	-D__FPU_PRESENT=1 \
	-ffreestanding \
	-fdata-sections \
	-ffunction-sections \
	-fno-inline \
	-fno-omit-frame-pointer

# =========================================================
# Includes
# =========================================================

INCLUDES := \
<<<<<<< Updated upstream
	-Icore \
	-Iapp \
	-Iconfig \
	-Ikernel/FreeRTOS/include \
	-Ikernel/FreeRTOS/portable/GCC/ARM_CM4F \
	-Idrivers/gpio \
	-Idrivers/systick \
	-Idrivers/i2c \
	-Idrivers/uart \
	-Idrivers/sensors/mpu6050 \
	-Idrivers/pwm \
	-Idrivers/adc \
	-Idrivers/timer \
	-Ilibs/CMSIS/Core/Include \
	-Ilibs/CMSIS/DSP/Include/dsp \
	-Ilibs/CMSIS/DSP/PrivateInclude \
	-Ilibs/CMSIS/DSP/Include
=======
    -Icore \
    -Iapp \
    -Iapp/gcs \
    -Iconfig \
    -Ikernel/FreeRTOS/include \
    -Ikernel/FreeRTOS/portable/GCC/ARM_CM4F \
    -Idrivers/gpio \
    -Idrivers/systick \
    -Idrivers/i2c \
    -Idrivers/spi \
    -Idrivers/uart \
    -Idrivers/sensors/mpu6050 \
    -Idrivers/sensors/bmp280 \
    -Idrivers/pwm \
    -Idrivers/adc \
    -Idrivers/timer \
    $(CMSIS_INCLUDES)
>>>>>>> Stashed changes


CFLAGS += $(INCLUDES)

# =========================================================
# Linker
# =========================================================

LDSCRIPT := core/linker_stm32f411.ld
<<<<<<< Updated upstream

LDFLAGS := $(CPUFLAGS)
=======
LDFLAGS := $(CPUFLAGS) -T$(LDSCRIPT) -nostdlib \
    -Wl,--gc-sections -Wl,-Map=$(MAP)
# Allocation-free string/memory routines come from libc; altitude uses libm.
# GCS has its own bounded formatter, with no printf, heap or syscall dependency.
LIBS := -Wl,--start-group -lm -lc -lgcc -Wl,--end-group

SRC := \
    core/startup_stm32f411.s \
    core/system.c \
    drivers/systick/systick.c \
    drivers/gpio/gpio.c \
    drivers/gpio/gpio_exti.c \
    drivers/i2c/i2c.c \
    drivers/spi/spi.c \
    drivers/uart/uart.c \
    drivers/pwm/pwm.c \
    drivers/adc/adc.c \
    drivers/timer/timer.c \
    drivers/sensors/mpu6050/mpu6050.c \
    drivers/sensors/bmp280/bmp280.c \
    app/task_imu.c \
    app/task_baro.c \
    app/task_list.c \
    app/gcs/gcs_link.c \
    app/gcs/gcs_format.c \
    app/gcs/task_gcs.c \
    $(CMSIS_DSP_SRC) \
    $(FREERTOS_SRC) \
    app/drone_app.c \
    app/main.c
>>>>>>> Stashed changes

LDFLAGS += \
	-T$(LDSCRIPT) \
	-nostdlib \
	-Wl,--gc-sections \
	-Wl,-Map=$(MAP) 

# =========================================================
# Libraries
# =========================================================

<<<<<<< Updated upstream
LIBS := \
	-lgcc


# =========================================================
# Sources
# =========================================================

SRC += \
	core/startup_stm32f411.s \
	core/system.c \
	drivers/systick/systick.c \
	drivers/gpio/gpio.c \
	drivers/i2c/i2c.c \
	drivers/uart/uart.c \
	drivers/pwm/pwm.c \
	drivers/adc/adc.c \
	drivers/timer/timer.c \
	drivers/sensors/mpu6050/mpu6050.c \
	app/task_imu.c \
	libs/CMSIS/DSP/Source/FastMathFunctions/arm_atan2_f32.c \
	libs/CMSIS/DSP/Source/FastMathFunctions/arm_sqrt_q31.c \
	$(FREERTOS_SRC) \
	app/drone_app.c \
	app/main.c

# =========================================================
# Objects
# =========================================================

OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.s=.o)

# =========================================================
# Default Target
# =========================================================
=======
.PHONY: all check-toolchain check-openocd flash debug gdb monitor test-i2c test-uart test-gpio test-pwm test-timer test-scheduler test-adc test-bmp280 test-spi test-gcs clean
>>>>>>> Stashed changes

all: $(BIN)

# =========================================================
# Compile C
# =========================================================

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# =========================================================
# Compile ASM
# =========================================================

%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CPUFLAGS) -c $< -o $@

# =========================================================
# Link ELF
# =========================================================

$(ELF): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(OBJ) $(LDFLAGS) $(LIBS) -o $@
	$(SIZE) $@

# =========================================================
# Generate BIN
# =========================================================

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# =========================================================
# Flash
# =========================================================

flash: $(ELF)
	openocd \
	-f interface/stlink.cfg \
	-f target/stm32f4x.cfg \
	-c "program $(ELF) verify reset exit"

# =========================================================
# OpenOCD Server
# =========================================================

debug:
	openocd -f scripts/openocd.cfg

<<<<<<< Updated upstream
# =========================================================
# GDB
# =========================================================

gdb:
	$(GDB) $(ELF)

# =========================================================
# Clean
# =========================================================
=======
# Linux host regression test using mapped peripheral registers.
HOST_CC ?= cc
test-gcs:
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Wshadow -Werror \
		-fsanitize=undefined -fno-sanitize-recover=undefined \
		-Iapp/gcs -Iapp -Iconfig -Icore -Idrivers/pwm -Idrivers/gpio \
		-Idrivers/uart -Idrivers/systick -Idrivers/sensors/bmp280 \
		tests/gcs_integration_test.c app/gcs/task_gcs.c app/gcs/gcs_link.c \
		app/gcs/gcs_format.c drivers/pwm/pwm.c drivers/gpio/gpio.c \
		-o $(BUILD_DIR)/test/gcs_integration_test -lm
	python3 tests/gcs_wire_test.py $(BUILD_DIR)/test/gcs_integration_test

test-i2c:
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Wshadow -Werror \
		-fsanitize=undefined -fno-sanitize-recover=undefined \
		-include tests/i2c_test_backend.h \
		-Idrivers/i2c -Idrivers/gpio -Idrivers/systick -Idrivers/uart \
		-Idrivers/sensors/mpu6050 -Icore \
		tests/i2c_driver_test.c drivers/i2c/i2c.c drivers/sensors/mpu6050/mpu6050.c \
		-o $(BUILD_DIR)/test/i2c_driver_test
	$(BUILD_DIR)/test/i2c_driver_test

test-uart: $(ELF)
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Wshadow -Werror -no-pie \
		-fsanitize=undefined -fno-sanitize-recover=undefined \
		-Idrivers/uart -Idrivers/gpio -Idrivers/systick \
		tests/uart_driver_test.c drivers/uart/uart.c drivers/gpio/gpio.c \
		-o $(BUILD_DIR)/test/uart_driver_test
	$(BUILD_DIR)/test/uart_driver_test
	$(OBJCOPY) -O binary --only-section=.text $(ELF) $(BUILD_DIR)/test/uart_text.bin
	python3 tests/gpio_vector_test.py $(BUILD_DIR)/test/uart_text.bin $(ELF) $(PREFIX)nm

test-gpio: $(ELF)
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Wshadow -Werror \
		-fsanitize=undefined -fno-sanitize-recover=undefined -Idrivers/gpio \
		tests/gpio_driver_test.c drivers/gpio/gpio.c drivers/gpio/gpio_exti.c \
		-o $(BUILD_DIR)/test/gpio_driver_test
	$(BUILD_DIR)/test/gpio_driver_test
	$(OBJCOPY) -O binary --only-section=.text $(ELF) $(BUILD_DIR)/test/gpio_text.bin
	python3 tests/gpio_vector_test.py $(BUILD_DIR)/test/gpio_text.bin $(ELF) $(PREFIX)nm

test-pwm:
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror \
		-Idrivers/pwm -Idrivers/gpio -Idrivers/uart -Idrivers/systick -Icore \
		tests/pwm_driver_test.c drivers/pwm/pwm.c drivers/gpio/gpio.c drivers/uart/uart.c \
		drivers/systick/systick.c \
		-o $(BUILD_DIR)/test/pwm_driver_test
	$(BUILD_DIR)/test/pwm_driver_test

test-timer: $(ELF)
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Idrivers/timer -Idrivers/gpio -Icore \
		tests/timer_driver_test.c drivers/timer/timer.c -o $(BUILD_DIR)/test/timer_driver_test
	$(BUILD_DIR)/test/timer_driver_test
	$(OBJCOPY) -O binary --only-section=.text $(ELF) $(BUILD_DIR)/test/timer_text.bin
	python3 tests/timer_vector_test.py $(BUILD_DIR)/test/timer_text.bin $(ELF) $(PREFIX)nm

test-adc:
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror \
		-Idrivers/adc -Idrivers/gpio -Idrivers/uart -Idrivers/pwm -Idrivers/systick -Icore \
		tests/adc_driver_test.c drivers/adc/adc.c drivers/gpio/gpio.c drivers/uart/uart.c drivers/pwm/pwm.c \
		-o $(BUILD_DIR)/test/adc_driver_test
	$(BUILD_DIR)/test/adc_driver_test

test-scheduler:
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Wshadow -Werror \
		-fsanitize=undefined -fno-sanitize-recover=undefined \
		-DDRONE_GCS_ENABLED=0 -Iapp -Iconfig -Icore -Idrivers/timer -Idrivers/gpio \
		-Idrivers/uart -Idrivers/sensors/bmp280 \
		tests/timer_scheduler_test.c drivers/timer/timer.c app/task_list.c \
		-o $(BUILD_DIR)/test/timer_scheduler_test
	$(BUILD_DIR)/test/timer_scheduler_test

test-bmp280:
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Wshadow -Werror \
		-Iapp -Idrivers/sensors/bmp280 -Idrivers/spi -Idrivers/gpio -Idrivers/systick -Idrivers/uart \
		tests/bmp280_driver_test.c drivers/sensors/bmp280/bmp280.c app/task_baro.c \
		-o $(BUILD_DIR)/test/bmp280_driver_test
	$(BUILD_DIR)/test/bmp280_driver_test

test-spi:
	@mkdir -p $(BUILD_DIR)/test
	$(HOST_CC) -std=c11 -Wall -Wextra -Wshadow -Werror \
		-Idrivers/spi -Idrivers/gpio -Idrivers/systick \
		tests/spi_driver_test.c drivers/spi/spi.c drivers/gpio/gpio.c \
		-o $(BUILD_DIR)/test/spi_driver_test
	$(BUILD_DIR)/test/spi_driver_test
>>>>>>> Stashed changes

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OBJ) *.elf *.bin *.map

# =========================================================
# Phony
# =========================================================

.PHONY: all flash debug gdb clean