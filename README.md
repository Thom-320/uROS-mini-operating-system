# uROS - Mini OS en RISC-V 64

> Un sistema operativo educativo minimalista para RISC-V 64 corriendo en QEMU

[![RISC-V](https://img.shields.io/badge/RISC--V-64-blue)](https://riscv.org/)
[![QEMU](https://img.shields.io/badge/QEMU-virt-orange)](https://www.qemu.org/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

## 🎯 Características

- ✅ **Arquitectura RISC-V 64** (rv64gc)
- ✅ **QEMU virt** con OpenSBI
- ✅ **Shell interactiva** con 10+ comandos
- ✅ **Sistema de tareas** con context switching
- ✅ **Scheduler Round-Robin** cooperativo
- ✅ **Driver UART** NS16550A
- ✅ **Printf** implementado
- ✅ **Gestión de memoria** con bump allocator

## 🚀 Inicio Rápido

### Prerequisitos

```bash
# macOS (con Homebrew)
brew install qemu riscv-gnu-toolchain

# Linux (Ubuntu/Debian)
sudo apt-get install qemu-system-misc gcc-riscv64-unknown-elf
```

### Compilar y Ejecutar

```bash
# 1. Compilar
make clean && make -j

# 2. Ejecutar
make run

# 3. En el prompt uROS>, prueba:
help
ps
run cpu
run io
ps
meminfo
```

**Para salir:** `Ctrl+C`

## 🎬 Demo Automática

```bash
./scripts/demo.sh
```

Ejecuta automáticamente: `help` → `ps` → `run cpu` → `run io` → `ps` → `sched rr` → `meminfo`

## 📋 Comandos Disponibles

| Comando | Descripción |
|---------|-------------|
| `help` | Lista todos los comandos |
| `ps` | Muestra tareas activas |
| `run cpu` | Crea tarea CPU-bound |
| `run io` | Crea tarea I/O-bound |
| `kill <pid>` | Termina una tarea |
| `sched rr` | Scheduler Round-Robin |
| `bench` | Benchmark (requiere timer) |
| `uptime` | Tiempo de ejecución |
| `meminfo` | Uso de memoria |

## 📁 Estructura

```
mini-os/
├── boot/start.S          # Boot y context switch
├── kernel/               # Core del kernel
│   ├── kmain.c          # Punto de entrada
│   ├── task.c           # Sistema de tareas
│   ├── sched.c          # Scheduler RR
│   ├── shell.c          # Shell interactiva
│   └── trap.c           # Interrupciones
├── drivers/             # Drivers de hardware
│   ├── uart.c           # NS16550A
│   └── timer.c          # Timer SBI
├── lib/printf.c         # Printf
├── include/uros.h       # Headers
└── scripts/             # Scripts útiles
```

## 🔧 Targets del Makefile

```bash
make            # Compila el proyecto
make run        # Ejecuta en QEMU
make run-gdb    # Ejecuta con debugger
make gdb        # Conecta GDB
make clean      # Limpia build/
make dtb        # Extrae device tree
```

## 📖 Documentación

- **[GUIA_COMPLETA.md](GUIA_COMPLETA.md)** - Guía detallada con ejemplos
- **[README_RAPIDO.md](README_RAPIDO.md)** - Referencia rápida
- **[VERIFICATION.md](VERIFICATION.md)** - Verificación técnica
- **[docs/README.md](docs/README.md)** - Documentación técnica

## 🎓 Ejemplo de Uso

```
$ make run

OpenSBI v1.5.1
[Boot info...]

uROS (rv64gc, QEMU virt) - console ready
ticks=0
Initializing task system...
Initializing scheduler...
Creating idle task...
System initialized, starting shell...

uROS> help
Available commands:
  help          - Show this help
  ps            - List tasks
  run cpu       - Create CPU-bound task
  ...

uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    0      1          0

uROS> run cpu
Created CPU task with PID 1

uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    0      1          0
1    READY    0      20          0
```

## 🐛 Debugging

```bash
# Terminal 1
make run-gdb

# Terminal 2
make gdb
(gdb) break kmain
(gdb) continue
(gdb) info registers
```

## ⚙️ Arquitectura

- **Base Address**: 0x80200000
- **Boot**: OpenSBI (-bios default)
- **Mode**: S-mode bare metal
- **UART**: 0x10000000 (NS16550A)
- **Scheduler**: Round-Robin cooperativo (sin timer por estabilidad)
- **Memory**: Bump allocator, 256KB heap
- **Tasks**: Max 32, 4KB stack cada una

## ✅ Estado del Proyecto

**Completamente funcional:**
- ✅ Boot y shell interactiva
- ✅ Sistema de tareas con context switching
- ✅ Scheduler Round-Robin cooperativo
- ✅ Todos los comandos básicos
- ✅ UART input/output
- ✅ Gestión de memoria

**Limitaciones actuales (por diseño):**
- Timer deshabilitado para estabilidad
- Sistema cooperativo (sin preemption automática)
- `bench` y `sched sjf` requieren timer

## 🤝 Colaboradores

- **Thomas** 
- **Simón** 
- **María Paula** 

## 📝 Licencia

MIT License - ver archivo LICENSE

---

**🚀 ¡Listo para demostración y evaluación!**

Para más detalles, consulta [GUIA_COMPLETA.md](GUIA_COMPLETA.md)

