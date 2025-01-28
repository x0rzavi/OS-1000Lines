#!/usr/bin/env bash
set -xue

# QEMU file path
QEMU='qemu-system-riscv32'
BIOS='./opensbi-riscv32-generic-fw_dynamic_v1.5.1.bin'

# Path to clang and compiler flags
CC=clang
OBJCOPY=llvm-objcopy
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib"

# Build the shell (application)
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf \
  shell.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin # convert to raw binary format
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o                    # convert to c-embeddable format

# Build the kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
  kernel.c common.c shell.bin.o # embed shell.bin.o into kernel

# Start QEMU
$QEMU -machine virt -bios $BIOS -nographic -serial mon:stdio --no-reboot \
  -d unimp,guest_errors,int,cpu_reset -D qemu.log \
  -kernel kernel.elf
