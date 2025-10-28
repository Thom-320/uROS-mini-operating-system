#!/bin/bash
echo "=== Test Directo de uROS ==="
echo "Lanzando QEMU directamente (sin make)..."
echo ""
echo "INSTRUCCIONES:"
echo "1. Cuando veas 'uROS>', escribe: help"
echo "2. Luego escribe: uptime"
echo "3. Para salir: Ctrl+A luego X"
echo ""
echo "Si no puedes escribir:"
echo "- Verifica que esta ventana tiene el foco"
echo "- Intenta hacer clic en la ventana"
echo "- Presiona Enter unas veces"
echo ""
read -p "Presiona Enter para continuar..."

exec qemu-system-riscv64 \
  -machine virt \
  -nographic \
  -bios default \
  -serial stdio \
  -monitor none \
  -kernel build/kernel.elf

