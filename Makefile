# ================= CONFIG =================
PROJECT = drone

CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m4 -mthumb -Wall -O0 -g
CFLAGS += -ffreestanding -nostdlib

LDFLAGS = -T linker.ld -nostdlib

# ================= INCLUDE =================
INCLUDES = \
-Icore \
-Idrivers/gpio \
-Idrivers/systick


SRC = \
    startup_stm32f411.s \
    core/system.c \
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

clean:
	rm -f $(OBJ) *.elf *.bin