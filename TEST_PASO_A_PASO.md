# 🔍 DEBUG PASO A PASO - uROS Input Problem

## 🎯 Objetivo

Aislar dónde exactamente falla el input del teclado.

---

## 📝 TEST 1: Echo Simple (AHORA ACTIVO)

### Qué Hace
- Solo UART, sin timer, sin scheduler, sin nada
- Loop infinito que hace echo de caracteres

### Ejecutar AHORA

```bash
cd "/Users/thom/Library/Mobile Documents/com~apple~CloudDocs/Universidad/OS/mini-os"
make run
```

### Qué Esperar

```
=== uROS UART Test ===
Test 1: Echo simple
Type characters (Ctrl+C to exit):

```

### AHORA: Escribe Algo

Presiona teclas en tu teclado: `a` `b` `c`

### Resultado Esperado

```
[got 0x61='a'] a[got 0x62='b'] b[got 0x63='c'] c
```

### ¿Funciona?

**SI VES caracteres echoed:**
✅ UART funciona correctamente
→ El problema está en el scheduler/timer/shell
→ Procede a TEST 2

**SI NO VES NADA:**
❌ UART no recibe datos
→ Problema en UART driver o QEMU
→ Reporta exactamente lo que ves

---

## 📝 TEST 2: Shell Simple (Sin Timer)

### Si TEST 1 Funciona

```bash
cp kernel/kmain_test2.c kernel/kmain.c
make clean && make -j
make run
```

### Qué Esperar

```
=== uROS Shell Test ===
Test 2: Simple shell (no timer, no scheduler)

uROS-test>
```

### AHORA: Escribe Comandos

```
help
test
echo hola
```

### Resultado Esperado

```
uROS-test> help
Commands: help, echo, test
uROS-test> test
Test command works!
uROS-test> echo hola
You said: hola
```

### ¿Funciona?

**SI FUNCIONA:**
✅ Shell básica funciona
→ El problema está en timer/scheduler
→ Procede a TEST 3

**SI NO FUNCIONA:**
❌ Problema en uart_gets
→ Reporta lo que pasa

---

## 📝 TEST 3: Con Scheduler (Sin Timer)

### Si TEST 2 Funciona

Edita `kernel/kmain_full.c` y comenta las líneas de timer:

```c
// kprintf("Initializing timer...\n");
// timer_init(100);
```

Luego:
```bash
cp kernel/kmain_full.c kernel/kmain.c
make clean && make -j
make run
```

### ¿Funciona?

**SI FUNCIONA:**
✅ El problema es el timer causando bloqueo
→ Necesitamos ajustar context switch
→ Procede a TEST 4

**SI NO FUNCIONA:**
❌ El problema es el scheduler
→ Reporta lo que ves

---

## 📝 TEST 4: Con Timer pero CONFIG_PREEMPT=0

### Si TEST 3 NO Funciona

Edita `include/config.h`:
```c
#define CONFIG_PREEMPT 0  // Deshabilitar preemption
```

Restaura kmain completo:
```bash
cp kernel/kmain_full.c kernel/kmain.c
make clean && make -j
make run
```

### ¿Funciona?

**SI FUNCIONA:**
✅ El problema es el context switch en modo preemptivo
→ Necesitamos revisar ctx_switch en boot/start.S

**SI NO FUNCIONA:**
❌ Otro problema
→ Necesitamos más info

---

## 🎯 AHORA - Ejecuta TEST 1

```bash
cd "/Users/thom/Library/Mobile Documents/com~apple~CloudDocs/Universidad/OS/mini-os"
make run
```

**Verás:**
```
=== uROS UART Test ===
Test 1: Echo simple
Type characters (Ctrl+C to exit):

```

**ESCRIBE:** Presiona `h`, luego `e`, luego `l`, luego `p`

**REPORTA:**
1. ¿Ves ALGO en pantalla cuando presionas teclas?
2. ¿Ves `[got 0x...='...']`?
3. ¿Ves el carácter echoed?
4. ¿O no pasa absolutamente nada?

**Por favor, ejecuta esto AHORA y dime exactamente qué ves.**

Basado en tu respuesta, sabremos si el problema es:
- UART driver
- uart_gets/shell
- scheduler/context switch
- timer interrupts

**Esperando tu reporte...** 🔍
