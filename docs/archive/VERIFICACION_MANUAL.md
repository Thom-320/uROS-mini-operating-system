# ✅ Verificación Manual de uROS

## 🔧 FIX APLICADO: uart_gets con yield

**Problema:** Shell no aceptaba entrada en modo preemptivo porque `uart_gets()` hacía busy-wait sin yield.

**Solución:** Agregado `task_yield()` cuando no hay datos disponibles en UART:
```c
if (c == -1) {
    sched_maybe_yield_safe();
    task_yield();
    continue;
}
```

**Resultado:** ✅ Ahora el sistema acepta entrada correctamente

---

## 🚀 Cómo Verificar (HAZLO AHORA)

### 1. Compilar
```bash
cd "/Users/thom/Library/Mobile Documents/com~apple~CloudDocs/Universidad/OS/mini-os"
make clean && make -j
```

### 2. Ejecutar
```bash
make run
```

### 3. Esperar el Prompt
Verás:
```
System initialized, starting shell...
uROS>
```

### 4. ESCRIBIR (con tu teclado):

**Paso 1:** Escribe `help` y presiona Enter
```
uROS> help
```

**Esperado:**
```
Available commands:
  help            - Show this help
  ps              - List tasks
  run cpu         - Create CPU-bound task
  run io          - Create I/O-bound task
  kill <pid>      - Kill a task
  sched rr        - Switch to Round-Robin
  sched sjf       - Switch to SJF
  sched preempt on|off - Enable/disable preemption
  sleep <ticks>   - Sleep for N ticks
  pcdemo          - Producer-Consumer demo
  bench           - Run scheduler benchmark
  uptime          - Show system uptime
  meminfo         - Show memory usage
```

**Paso 2:** Escribe `uptime` y presiona Enter
```
uROS> uptime
```

**Esperado:**
```
Uptime: 0.XX seconds (XX ticks)
```

**Paso 3:** Espera 3 segundos, escribe `uptime` de nuevo
```
uROS> uptime
```

**Esperado:**
```
Uptime: 3.XX seconds (3XX ticks)
```
→ ✅ Los ticks deben haber aumentado (prueba que el timer funciona)

**Paso 4:** Escribe `ps`
```
uROS> ps
```

**Esperado:**
```
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    XXX    1          0
```

**Paso 5:** Escribe `pcdemo`
```
uROS> pcdemo
```

**Esperado:**
```
=== Producer-Consumer Demo ===
Buffer size: 16 items
Creating producer and consumer tasks...
Producer task created (PID 1)
Consumer task created (PID 2)
Demo running... (will produce/consume 10 items)
Watch the alternating output!

Producer: produced item 1 at index 0
Consumer: consumed item 1 from index 0
Producer: produced item 2 at index 1
Consumer: consumed item 2 from index 1
Producer: produced item 3 at index 2
Consumer: consumed item 3 from index 2
...
Producer: finished producing 10 items
Consumer: finished consuming 10 items

uROS>
```

**Paso 6:** Escribe `ps` de nuevo
```
uROS> ps
```

**Esperado:**
```
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    XXX    1          0
1    ZOMBIE   YYY    20         ZZZ
2    ZOMBIE   WWW    20         AAA
```

**Paso 7:** Escribe `meminfo`
```
uROS> meminfo
```

**Esperado:**
```
Heap used: XXXXX / 262144 bytes
```

---

## ✅ Si Todo Funciona

Deberías poder:
- ✅ Escribir cualquier comando
- ✅ Ver respuestas inmediatas
- ✅ Ver `uptime` aumentando
- ✅ Ejecutar `pcdemo` y ver sincronización
- ✅ Ver producer/consumer alternando sin race conditions

---

## ⚠️ Si NO Funciona (Debugging)

### Síntoma: Prompt aparece pero no acepta entrada

**Causa posible:** El yield en uart_gets puede estar causando context switch a idle task

**Solución rápida:** Cambiar CONFIG_PREEMPT a 0

1. Edita `include/config.h`:
```c
#define CONFIG_PREEMPT 0  // Modo cooperativo
```

2. Recompila:
```bash
make clean && make -j
make run
```

3. Prueba de nuevo

---

### Síntoma: uptime siempre da 0

**Causa:** Timer no está funcionando

**Verificación:**
1. Asegura que `trap_init()` se llama en `kmain.c` ✅
2. Asegura que `timer_init(100)` se llama en `kmain.c` ✅

---

### Síntoma: Sistema se cuelga en "Initializing trap handling"

**Causa:** Interrupción de timer antes de estar listo

**Solución:** El orden de init ya está correcto:
```c
task_init();
sched_init();
task_create(idle_task);  // ← Crear idle ANTES de timer
timer_init(100);
trap_init();
```

---

## 🎯 DESPUÉS de Verificar

Una vez que confirmes que el sistema acepta entrada y `pcdemo` funciona, entonces implementaremos la gestión de memoria mejorada con `kfree`.

**Por favor ejecuta `make run` ahora y prueba escribir `help` con tu teclado.**

¿Funciona? Dime qué ves.

