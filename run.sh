#!/usr/bin/env bash
set -xue

# QEMU file path
QEMU='qemu-system-riscv32'
BIOS='./opensbi-riscv32-generic-fw_dynamic_v1.5.1.bin'

# Path to clang and compiler flags
CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib"

# Build the kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
  kernel.c common.c

# Start QEMU
$QEMU -machine virt -bios $BIOS -nographic -serial mon:stdio --no-reboot \
  -kernel kernel.elf
