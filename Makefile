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
	-ffreestanding \
	-fdata-sections \
	-ffunction-sections \
	-fno-inline \
	-fno-omit-frame-pointer

# =========================================================
# Includes
# =========================================================

INCLUDES := \
	-Icore \
	-Iapp \
	-Idrivers/gpio \
	-Idrivers/systick \
	-Idrivers/i2c \
	-Idrivers/uart \
	-Idrivers/mpu6050 \
	-Idrivers/pwm \
	-Idrivers/adc \
	-Idrivers/timer

CFLAGS += $(INCLUDES)

# =========================================================
# Linker
# =========================================================

LDSCRIPT := core/linker_stm32f411.ld

LDFLAGS := $(CPUFLAGS)

LDFLAGS += \
	-T$(LDSCRIPT) \
	-nostdlib \
	-Wl,--gc-sections \
	-Wl,-Map=$(MAP) \
	-lgcc

# =========================================================
# Sources
# =========================================================

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
	drivers/mpu6050/mpu6050.c \
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
	$(LD) $(OBJ) $(LDFLAGS) -o $@
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
	-f scripts/openocd.cfg \
	-c "program $(ELF) verify reset exit"

# =========================================================
# OpenOCD Server
# =========================================================

debug:
	openocd -f scripts/openocd.cfg

# =========================================================
# GDB
# =========================================================

gdb:
	$(GDB) $(ELF)

# =========================================================
# Clean
# =========================================================

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OBJ) *.elf *.bin *.map

# =========================================================
# Phony
# =========================================================

.PHONY: all flash debug gdb clean