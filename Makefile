# ================= CONFIG =================
PROJECT = drone

CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m4 -mthumb -Wall -O0 -g3
CFLAGS += -ffreestanding -nostdlib
CFLAGS += -fno-inline -fno-omit-frame-pointer
CFLAGS += $(INCLUDES)
LDFLAGS = -T linker.ld -nostdlib

# ================= INCLUDE =================
INCLUDES = \
-Icore \
-Idrivers/gpio \
-Idrivers/systick\
-Idrivers/i2c


SRC = \
    startup_stm32f411.s \
    core/system.c \
    drivers/systick/systick.c \
    drivers/gpio/gpio.c \
	drivers/i2c/i2c.c \
    app/main.c

OBJ = $(SRC:.c=.o)
OBJ := $(OBJ:.s=.o)

all: $(PROJECT).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

$(PROJECT).elf: $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

FLASH_ADDR = 0x08000000

flash: $(PROJECT).elf
	openocd -f scripts/openocd.cfg \
	-c "program $(PROJECT).elf verify reset exit"

debug:
	openocd -f scripts/openocd.cfg

clean:
	rm -f $(OBJ) *.elf *.bin