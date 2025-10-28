#!/bin/bash
# uROS Demo Script
# Ejecuta una secuencia de comandos para demostrar el sistema

echo "=== uROS Demo Script ==="
echo "Running commands: help, ps, run cpu, run io, ps, sched rr, meminfo"
echo ""

(
  sleep 1
  echo "help"
  sleep 1
  echo "ps"
  sleep 1
  echo "run cpu"
  sleep 1
  echo "run io"
  sleep 1
  echo "ps"
  sleep 1
  echo "sched rr"
  sleep 1
  echo "meminfo"
  sleep 2
) | ./scripts/run-qemu.sh