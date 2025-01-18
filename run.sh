#!/usr/bin/env bash
set -xue

# QEMU file path
QEMU='qemu-system-riscv32'
BIOS='/usr/lib/riscv64-linux-gnu/opensbi/generic/fw_dynamic.bin'

# Path to clang and compiler flags
CC=clang # Ubuntu users: use CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib"

# Build the kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
  kernel.c

# Start QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
  -kernel kernel.elf
