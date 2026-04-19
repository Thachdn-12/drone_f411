set pagination off
set confirm off

# connect to openocd
target remote localhost:3333

# reset + halt MCU
monitor reset halt

# load firmware (optional)
# monitor program build.elf verify reset

# break at main
break main

# pretty print
set print pretty on

# define shortcut
define regs
    info registers
end

define rcc
    x/1x 0x40023800
end

define gpioa
    x/10x 0x40020000
end

define usart2
    x/5x 0x40004400
end

echo ✅ GDB ready - STM32F411 debug\n