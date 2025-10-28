CROSS = riscv64-unknown-elf-
CC = $(CROSS)gcc
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump

CFLAGS = -march=rv64gc -mabi=lp64 -mcmodel=medany -ffreestanding -nostdlib -nostartfiles \
         -Wall -Wextra -O2 -Iinclude -g

LDFLAGS = -T linker.ld -nostdlib -Wl,-Map=build/kernel.map

# Source files
SRC_C = kernel/kmain.c kernel/trap.c kernel/task.c kernel/sched.c kernel/shell.c kernel/sync.c kernel/kmem.c \
        drivers/uart.c drivers/timer.c \
        lib/printf.c

SRC_S = boot/start.S

# Object files
OBJ_C = $(patsubst %.c,build/%.o,$(SRC_C))
OBJ_S = $(patsubst %.S,build/%.o,$(SRC_S))
OBJ = $(OBJ_C) $(OBJ_S)

# Targets
.PHONY: all run clean dtb

all: build/kernel.elf

build/kernel.elf: $(OBJ) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)
	$(OBJDUMP) -D $@ > build/kernel.asm

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: build/kernel.elf
	@bash scripts/run-qemu.sh

clean:
	rm -rf build/

dtb:
	qemu-system-riscv64 -machine virt,dumpdtb=virt.dtb -nographic -bios default
	dtc -I dtb -O dts virt.dtb -o virt.dts
	@echo "DTB dumped to virt.dtb and virt.dts"

