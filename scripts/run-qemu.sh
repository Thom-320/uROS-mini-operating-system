#!/bin/bash
set -euo pipefail

QEMU_CMD="qemu-system-riscv64 \
  -machine virt \
  -nographic \
  -bios default \
  -serial stdio \
  -monitor none \
  -kernel build/kernel.elf"

if [ "${1:-}" = "gdb" ]; then
  echo "Starting QEMU in GDB mode on :1234 ..."
  exec $QEMU_CMD -S -s
else
  exec $QEMU_CMD
fi