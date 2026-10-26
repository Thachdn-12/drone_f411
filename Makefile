# =========================================================
# Drone F411 - STM32F411CEU6 (Cortex-M4F)
# =========================================================
PROJECT   := drone
BUILD_DIR := output
ELF       := $(BUILD_DIR)/$(PROJECT).elf
BIN       := $(BUILD_DIR)/$(PROJECT).bin
MAP       := $(BUILD_DIR)/$(PROJECT).map

# Use the repository-local toolchain if present, otherwise search PATH.
# Override with: make TOOLCHAIN_DIR=/path/to/toolchain/bin
LOCAL_TOOLCHAIN_DIR := toolchains/bin
TOOLCHAIN_DIR ?= $(if $(wildcard $(LOCAL_TOOLCHAIN_DIR)/arm-none-eabi-gcc),$(LOCAL_TOOLCHAIN_DIR))
PREFIX        := $(if $(strip $(TOOLCHAIN_DIR)),$(patsubst %/,%,$(TOOLCHAIN_DIR))/)arm-none-eabi-
CC            := $(PREFIX)gcc
LD            := $(PREFIX)gcc
OBJCOPY       := $(PREFIX)objcopy
SIZE          := $(PREFIX)size
GDB           := $(PREFIX)gdb

# Prefer the xPack OpenOCD bundled in tools/, otherwise search PATH.
OPENOCD_DIR     ?= tools/xpack-openocd-0.12.0-7
OPENOCD         ?= $(if $(wildcard $(OPENOCD_DIR)/bin/openocd),$(OPENOCD_DIR)/bin/openocd,openocd)
OPENOCD_SCRIPTS ?= $(if $(wildcard $(OPENOCD_DIR)/openocd/scripts),$(OPENOCD_DIR)/openocd/scripts)
OPENOCD_INTERFACE ?= interface/stlink.cfg
OPENOCD_TARGET    ?= target/stm32f4x.cfg
SWD_SPEED        ?= 1000
GDB_PORT         ?= 3333
PORT             ?= /dev/ttyUSB0
BAUD             ?= 115200

# Select SWD explicitly; use software reset so an NRST wire is optional.
OPENOCD_FLAGS = $(if $(strip $(OPENOCD_SCRIPTS)),-s "$(OPENOCD_SCRIPTS)") \
    -f "$(OPENOCD_INTERFACE)" -c "transport select swd" \
    -f "$(OPENOCD_TARGET)" -c "adapter speed $(SWD_SPEED)" \
    -c "reset_config none" -c "gdb port $(GDB_PORT)"

CPUFLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CMSIS_DIR        := libs/CMSIS
CMSIS_DRIVER_DIR := $(CMSIS_DIR)/Driver
CMSIS_DEVICE_DIR := $(CMSIS_DRIVER_DIR)/Device/ST/STM32F4xx
CMSIS_DSP_DIR    := $(CMSIS_DIR)/DSP

# CMSIS Driver currently contains headers only. Keep our own startup/system.c.
CMSIS_INCLUDES := \
    -I$(CMSIS_DEVICE_DIR)/Include \
    -I$(CMSIS_DRIVER_DIR)/Include \
    -I$(CMSIS_DIR)/Core/Include \
    -I$(CMSIS_DSP_DIR)/Include \
    -I$(CMSIS_DSP_DIR)/PrivateInclude

# List individual DSP implementations, not the aggregate FastMathFunctions.c,
# to avoid duplicate symbols. arm_sqrt_f32 is provided inline by DSP headers.
CMSIS_DSP_SRC := \
    $(CMSIS_DSP_DIR)/Source/FastMathFunctions/arm_atan2_f32.c

FREERTOS_SRC := \
    kernel/FreeRTOS/tasks.c \
    kernel/FreeRTOS/queue.c \
    kernel/FreeRTOS/list.c \
    kernel/FreeRTOS/portable/GCC/ARM_CM4F/port.c \
    kernel/FreeRTOS/portable/MemMang/heap_4.c

INCLUDES := \
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
    $(CMSIS_INCLUDES)

CPPFLAGS += -DSTM32F411xE -DARM_MATH_CM4 -D__FPU_PRESENT=1U $(INCLUDES)

CFLAGS := $(CPUFLAGS) \
    -std=c11 -Wall -Wextra -Wshadow -Wundef \
    -O0 -g3 \
    -ffreestanding -fdata-sections -ffunction-sections \
    -fno-inline -fno-omit-frame-pointer \
    -MMD -MP

LDSCRIPT := core/linker_stm32f411.ld
LDFLAGS := $(CPUFLAGS) -T$(LDSCRIPT) -nostdlib \
    -Wl,--gc-sections -Wl,-Map=$(MAP)
LIBS := -lgcc

SRC := \
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
    $(CMSIS_DSP_SRC) \
    $(FREERTOS_SRC) \
    app/drone_app.c \
    app/main.c

OBJ := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(filter %.c,$(SRC))) \
       $(patsubst %.s,%.o,$(filter %.s,$(SRC))))
DEP := $(OBJ:.o=.d)

.DEFAULT_GOAL := all

.PHONY: all check-toolchain check-openocd flash debug gdb monitor clean

all: $(BIN)

check-toolchain:
	@command -v "$(CC)" >/dev/null 2>&1 || { \
	  echo "ERROR: ARM GCC not found: $(CC)"; \
	  echo "Install ARM GNU Toolchain on PATH or extract it into toolchains/"; \
	  echo "Or run: make TOOLCHAIN_DIR=/path/to/toolchain/bin"; \
	  exit 1; \
	}

check-openocd:
	@command -v "$(OPENOCD)" >/dev/null 2>&1 || { \
	  echo "ERROR: OpenOCD not found: $(OPENOCD)"; \
	  echo "Run: make flash OPENOCD_DIR=/path/to/xpack-openocd"; \
	  echo "Or set OPENOCD=/path/to/openocd OPENOCD_SCRIPTS=/path/to/scripts"; \
	  exit 1; \
	}

$(BUILD_DIR)/%.o: %.c Makefile | check-toolchain
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | check-toolchain
	@mkdir -p $(dir $@)
	$(CC) $(CPUFLAGS) -c $< -o $@

$(ELF): $(OBJ) $(LDSCRIPT) | check-toolchain
	@mkdir -p $(BUILD_DIR)
	$(LD) $(OBJ) $(LDFLAGS) $(LIBS) -o $@
	$(SIZE) $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

flash: check-openocd $(ELF)
	"$(OPENOCD)" $(OPENOCD_FLAGS) \
		-c "program {$(ELF)} verify reset exit"

debug: check-openocd
	"$(OPENOCD)" $(OPENOCD_FLAGS)

gdb: $(ELF)
	"$(GDB)" -q -nx "$(ELF)" \
		-ex "target remote localhost:$(GDB_PORT)" \
		-ex "monitor reset halt" -ex "tbreak main" -ex "continue"

monitor:
	@command -v screen >/dev/null 2>&1 || { echo "Install screen: sudo apt install screen"; exit 1; }
	@test -c "$(PORT)" || { echo "Serial port not found: $(PORT). Set PORT=/dev/ttyUSBx"; exit 1; }
	stty -F "$(PORT)" $(BAUD) raw -echo cs8 -parenb -cstopb -crtscts -ixon -ixoff
	screen -fn "$(PORT)" "$(BAUD)"

clean:
	rm -rf $(BUILD_DIR)

-include $(DEP)
