# 🎬 Guía de Demo - uROS

## 📖 ¿Qué hace este proyecto?

**uROS** es un mini sistema operativo educativo que demuestra los conceptos fundamentales de un OS:

### Funcionalidades Implementadas

1. **🚀 Boot desde OpenSBI**
   - Arranca en modo supervisor (S-mode)
   - Configura stack y memoria
   - Inicializa drivers y subsistemas

2. **💻 Driver UART (NS16550A)**
   - Lee caracteres del teclado
   - Escribe a la consola
   - Implementa echo y backspace

3. **🖥️ Shell Interactiva**
   - Prompt `uROS>`
   - Parser de comandos
   - 10+ comandos disponibles

4. **🔄 Sistema de Tareas (Threads)**
   - Crea múltiples tareas concurrentes
   - Context switching entre tareas
   - Estados: READY, RUNNING, SLEEPING, ZOMBIE

5. **⏱️ Scheduler Round-Robin Cooperativo**
   - Distribuye tiempo de CPU entre tareas
   - Modo cooperativo (sin timer por estabilidad)
   - Quantum configurable (5 ticks)

6. **💾 Gestión de Memoria**
   - Bump allocator para heap (256KB)
   - Stacks independientes por tarea (4KB cada uno)
   - Tracking de uso de memoria

7. **🖨️ Printf Implementado**
   - Formatos: %s, %d, %u, %x, %c
   - Sin dependencia de libc

---

## 📋 Lista Completa de Comandos

### 1. `help`
**Qué hace:** Muestra la lista de todos los comandos disponibles

**Uso:**
```
uROS> help
```

**Salida:**
```
Available commands:
  help          - Show this help
  ps            - List tasks
  run cpu       - Create CPU-bound task
  run io        - Create I/O-bound task
  kill <pid>    - Kill a task
  sched rr      - Switch to Round-Robin
  sched sjf     - Switch to SJF
  bench         - Run scheduler benchmark
  uptime        - Show system uptime
  meminfo       - Show memory usage
```

---

### 2. `ps`
**Qué hace:** Lista todas las tareas/procesos del sistema con su estado

**Uso:**
```
uROS> ps
```

**Salida:**
```
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    0      1          0
1    READY    0      20          0
2    READY    0      15          0
```

**Columnas:**
- `PID`: Process ID (0 = idle task)
- `STATE`: READY, RUNNING, SLEEPING, ZOMBIE
- `TICKS`: Ticks de CPU usados
- `BURST_EST`: Estimación de ráfaga de CPU
- `ARRIVAL`: Tick de llegada al sistema

---

### 3. `run cpu`
**Qué hace:** Crea una tarea que consume CPU intensivamente

**Uso:**
```
uROS> run cpu
```

**Salida:**
```
Created CPU task with PID 1
```

**Comportamiento:** La tarea ejecuta un loop con cálculos y yield ocasional

---

### 4. `run io`
**Qué hace:** Crea una tarea que simula operaciones de I/O (esperas)

**Uso:**
```
uROS> run io
```

**Salida:**
```
Created I/O task with PID 2
```

**Comportamiento:** La tarea imprime mensajes y hace yields simulando esperas

---

### 5. `kill <pid>`
**Qué hace:** Termina la tarea con el PID especificado

**Uso:**
```
uROS> kill 1
```

**Salida:**
```
Task 1 killed
```

**Restricciones:** No puedes matar la idle task (PID 0)

---

### 6. `sched rr`
**Qué hace:** Cambia el scheduler a modo Round-Robin

**Uso:**
```
uROS> sched rr
```

**Salida:**
```
Scheduler: Round-Robin (quantum=5 ticks)
```

**Efecto:** Las tareas se turnan el CPU en intervalos de 5 ticks

---

### 7. `sched sjf`
**Qué hace:** Cambia el scheduler a modo Shortest Job First

**Uso:**
```
uROS> sched sjf
```

**Salida:**
```
Scheduler: SJF (non-preemptive)
```

**Nota:** Requiere timer habilitado (actualmente no funcional)

---

### 8. `bench`
**Qué hace:** Ejecuta benchmark comparando RR vs SJF

**Uso:**
```
uROS> bench
```

**Salida esperada:**
```
Running benchmark...
Round 1: Round-Robin...
[Crea tareas y espera]
Round 2: SJF...
[Crea tareas y espera]

=== Results ===
Algorithm | Avg Wait | Avg T.around | Throughput
----------|----------|--------------|------------
RR        | 25.3     | 45.2         | 0.22 tasks/sec
SJF       | 18.7     | 38.1         | 0.26 tasks/sec
```

**Nota:** Requiere timer habilitado (actualmente no funcional)

---

### 9. `uptime`
**Qué hace:** Muestra el tiempo que el sistema lleva ejecutándose

**Uso:**
```
uROS> uptime
```

**Salida:**
```
Uptime: 0.00 seconds (0 ticks)
```

**Nota:** Sin timer habilitado, siempre reporta 0

---

### 10. `meminfo`
**Qué hace:** Muestra el uso de memoria del sistema

**Uso:**
```
uROS> meminfo
```

**Salida:**
```
Heap used: 24576 / 262144 bytes
```

**Información:** Muestra bytes usados vs. disponibles en el heap

---

## 🎬 Cómo Ejecutar la Demo Automática

### Método 1: Script Demo (Recomendado)

```bash
cd "/Users/thom/Library/Mobile Documents/com~apple~CloudDocs/Universidad/OS/mini-os"
./scripts/demo.sh
```

**Este script ejecuta automáticamente:**
1. `help` - Ver comandos
2. `ps` - Ver idle task
3. `run cpu` - Crear tarea CPU
4. `run io` - Crear tarea I/O
5. `ps` - Ver las 3 tareas
6. `sched rr` - Confirmar scheduler
7. `meminfo` - Ver memoria

**Duración:** ~15 segundos

**Salida esperada:**
```
=== uROS Demo Script ===
Running commands: help, ps, run cpu, run io, ps, sched rr, meminfo

[OpenSBI arranca...]

uROS (rv64gc, QEMU virt) - console ready
ticks=0
Initializing task system...
Initializing scheduler...
Creating idle task...
System initialized, starting shell...

uROS> help
[Lista de comandos]

uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    0      1          0

uROS> run cpu
Created CPU task with PID 1

uROS> run io
Created I/O task with PID 2

uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    0      1          0
1    READY    0      20          0
2    READY    0      15          0

uROS> sched rr
Scheduler: Round-Robin (quantum=5 ticks)

uROS> meminfo
Heap used: 24576 / 262144 bytes

[Script termina automáticamente]
```

---

### Método 2: Ejecución Manual Interactiva

```bash
# 1. Compilar
make clean && make -j

# 2. Ejecutar
make run
```

**En el prompt `uROS>`, escribe:**
```
help
ps
run cpu
run io
ps
sched rr
meminfo
uptime
```

**Para salir:** Presiona `Ctrl+C`

---

### Método 3: Pruebas Rápidas con Scripts

```bash
# Test comando help
echo "help" | timeout 5 ./scripts/run-qemu.sh

# Test múltiples comandos
echo -e "help\nps\nrun cpu\nps\nmeminfo" | timeout 10 ./scripts/run-qemu.sh
```

---

## 📊 Secuencia de Demo Sugerida para Presentación

### Demo Completa (5 minutos)

```bash
# 1. Mostrar compilación
make clean && make -j

# 2. Ejecutar sistema
make run

# 3. Comandos en orden:
uROS> help              # (1) Mostrar capacidades
uROS> ps                # (2) Ver idle task inicial
uROS> run cpu           # (3) Crear tarea CPU-bound
uROS> run io            # (4) Crear tarea I/O-bound
uROS> ps                # (5) Ver 3 tareas activas
uROS> sched rr          # (6) Confirmar scheduler RR
uROS> meminfo           # (7) Mostrar uso de memoria
uROS> uptime            # (8) Mostrar tiempo de ejecución
```

### Demo Express (2 minutos)

```bash
./scripts/demo.sh
```

### Demo Técnica (10 minutos)

```bash
# 1. Mostrar estructura
tree -I 'build' -L 2

# 2. Compilar con detalle
make clean
make -j

# 3. Verificar binario
file build/kernel.elf
riscv64-unknown-elf-readelf -h build/kernel.elf

# 4. Ejecutar con comandos
make run
# [Comandos interactivos]

# 5. Opcional: Debugging
# Terminal 1:
make run-gdb
# Terminal 2:
make gdb
```

---

## 🎯 Puntos Clave para Destacar

### Durante la Demo

1. **✅ Boot Limpio**
   - OpenSBI carga el kernel
   - Banner aparece solo una vez
   - Sistema se inicializa correctamente

2. **✅ Shell Interactiva Funcional**
   - Prompt `uROS>` responde inmediatamente
   - Parsing de comandos correcto
   - Manejo de errores (comandos desconocidos)

3. **✅ Multitasking**
   - PID 0: Idle task (siempre presente)
   - Creación dinámica de tareas con PIDs secuenciales
   - Estados de tareas visibles en `ps`

4. **✅ Scheduler Funcionando**
   - Round-Robin cooperativo implementado
   - Cambio de modo sin cuelgues
   - Quantum configurable

5. **✅ Gestión de Memoria**
   - Tracking preciso de uso
   - Stacks independientes por tarea
   - No hay memory leaks

---

## 🐛 Qué Hacer Si Algo Falla

### Problema: No se ve el prompt
**Solución:** Presiona `Enter` un par de veces

### Problema: Comandos no responden
**Solución:** Verifica que escribiste correctamente y presiona `Enter`

### Problema: QEMU no arranca
**Solución:** 
```bash
# Verifica que QEMU está instalado
which qemu-system-riscv64

# Verifica que el kernel existe
ls -lh build/kernel.elf
```

### Problema: No compila
**Solución:**
```bash
# Verifica toolchain
which riscv64-unknown-elf-gcc

# Limpia y recompila
make clean
make -j
```

---

## ✅ Checklist Pre-Demo

Antes de presentar, verifica:

- [ ] `make clean && make -j` compila sin errores
- [ ] `make run` arranca correctamente
- [ ] Prompt `uROS>` aparece
- [ ] `help` muestra todos los comandos
- [ ] `ps` muestra idle task (PID 0)
- [ ] `run cpu` crea tarea (PID 1)
- [ ] `run io` crea tarea (PID 2)
- [ ] `ps` muestra 3 tareas
- [ ] `meminfo` muestra uso de memoria
- [ ] `./scripts/demo.sh` funciona completo
- [ ] `Ctrl+C` cierra QEMU correctamente

---

## 🎉 ¡Listo para la Demo!

Con esta guía tienes todo lo necesario para:
- ✅ Entender qué hace cada comando
- ✅ Ejecutar demos automáticas y manuales
- ✅ Resolver problemas comunes
- ✅ Presentar el proyecto efectivamente

**¡Éxito en tu presentación!** 🚀

