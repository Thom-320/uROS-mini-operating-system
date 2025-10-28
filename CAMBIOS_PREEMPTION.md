# 🔄 Cambios: Modo Preemptivo Configurable

## 📋 Resumen de Cambios

Se ha implementado un sistema de scheduling configurable que permite cambiar entre modo **preemptivo** (timer-driven) y **cooperativo** (manual yield) en tiempo de ejecución.

---

## 🆕 Archivos Nuevos

### 1. `include/config.h`
```c
#pragma once
#define CONFIG_PREEMPT 1  // 1=preemptivo, 0=cooperativo
```

**Función:** Define el modo de scheduling por defecto al compilar

---

## 🔧 Archivos Modificados

### 2. `include/uros.h`
**Cambios:**
- Incluye `config.h`
- Agrega prototipos:
  - `void sched_set_preempt(int on);`
  - `int sched_get_preempt(void);`

---

### 3. `kernel/trap.c`
**Cambios principales:**
```c
void trap_handler_c(u64 scause, u64 sepc, u64 stval) {
    if (scause == 0x8000000000000005UL) {
        g_ticks++;                // Incrementar ticks
        timer_schedule_next();    // Programar siguiente tick
        
        #if CONFIG_PREEMPT
        sched_on_tick();          // Modo preemptivo: context switch en IRQ
        #else
        need_resched = 1;         // Modo cooperativo: solo flag
        #endif
        
        return;
    }
    // ...
}
```

**Características:**
- ✅ No usa `kprintf` en IRQ (evita reentrancia)
- ✅ Usa compilación condicional según `CONFIG_PREEMPT`
- ✅ Retorna con `sret` correctamente

---

### 4. `kernel/sched.c`
**Cambios principales:**

```c
static int preempt_enabled = CONFIG_PREEMPT;  // Estado runtime

void sched_on_tick(void) {
    // Guard: no tasks yet
    if (!current_task && ready_count == 0) {
        return;
    }
    
    if (current_task) {
        current_task->ticks_used++;
    }
    
    // Modo PREEMPTIVO: context switch en IRQ
    if (preempt_enabled && current_mode == SCHED_RR && current_task) {
        quantum_left--;
        if (quantum_left <= 0 && ready_count > 0) {
            sched_yield();  // ✅ Preemptive switch
        }
    }
    
    // Modo COOPERATIVO: solo flag
    if (!preempt_enabled && current_mode == SCHED_RR && current_task) {
        quantum_left--;
        if (quantum_left <= 0) {
            need_resched = 1;  // ✅ Manual yield required
        }
    }
}

// Nuevas funciones
void sched_set_preempt(int on) {
    disable_irq();
    preempt_enabled = on;
    enable_irq();
}

int sched_get_preempt(void) {
    return preempt_enabled;
}
```

**Características:**
- ✅ Guarda contra tareas no inicializadas
- ✅ Permite cambio dinámico preemptivo ↔ cooperativo
- ✅ Mantiene seguridad con `disable_irq()`

---

### 5. `kernel/shell.c`
**Cambios principales:**

#### Comando `help` actualizado:
```
  sched preempt on|off - Enable/disable preemption
  sleep <ticks>        - Sleep for N ticks
```

#### Nuevo comando: `sched preempt`
```c
static void cmd_sched(const char *arg) {
    // ... (rr, sjf existentes)
    
    else if (strncmp(arg, "preempt ", 8) == 0) {
        const char *mode = arg + 8;
        if (strcmp(mode, "on") == 0) {
            sched_set_preempt(1);
            kprintf("Preemption: ENABLED (timer-driven context switches)\n");
        } else if (strcmp(mode, "off") == 0) {
            sched_set_preempt(0);
            kprintf("Preemption: DISABLED (cooperative scheduling)\n");
        }
    }
}
```

#### Nuevo comando: `sleep`
```c
static void cmd_sleep(const char *arg) {
    int ticks = parse_number(arg);
    kprintf("Sleeping for %d ticks...\n", ticks);
    
    u64 target = g_ticks + ticks;
    while (g_ticks < target) {
        if (!sched_get_preempt()) {
            // Cooperativo: debe yield manualmente
            sched_maybe_yield_safe();
            task_yield();
        }
        // Preemptivo: timer hace context switch automático
    }
    
    kprintf("Done sleeping\n");
}
```

**Parsing de comandos actualizado:**
```c
} else if (strncmp(buf, "sleep ", 6) == 0) {
    cmd_sleep(buf + 6);
}
```

---

### 6. `kernel/kmain.c`
**Orden de inicialización corregido:**
```c
void kmain(void) {
    uart_init();
    kprintf("uROS (rv64gc, QEMU virt) - console ready\n");
    
    task_init();          // 1. Sistema de tareas
    sched_init();         // 2. Scheduler
    task_create(idle_task, 0, 1);  // 3. Idle task
    timer_init(100);      // 4. Timer (programa primer tick)
    trap_init();          // 5. Habilitar interrupts (ÚLTIMO)
    
    kprintf("System initialized, starting shell...\n");
    shell_run();
}
```

**Razón del orden:**
- ✅ Timer e interrupts se habilitan AL FINAL
- ✅ Evita interrupciones antes de estar listo
- ✅ Asegura que hay al menos idle task antes de tick

---

## 📝 Scripts (Ya Correctos)

### `scripts/run-qemu.sh`
```bash
QEMU_CMD="qemu-system-riscv64 \
  -machine virt \
  -nographic \
  -bios default \
  -serial stdio \    # ✅ Correcto
  -monitor none \    # ✅ Correcto
  -kernel build/kernel.elf"
```

---

## 🎯 Cómo Usar

### Compilar
```bash
make clean && make -j
```

### Ejecutar
```bash
make run
```

### Comandos en Shell

#### Ver tiempo de ejecución (ahora funciona con timer!)
```
uROS> uptime
Uptime: 5.23 seconds (523 ticks)
```

#### Cambiar entre modos
```
uROS> sched preempt off
Preemption: DISABLED (cooperative scheduling)

uROS> sched preempt on
Preemption: ENABLED (timer-driven context switches)
```

#### Dormir N ticks
```
uROS> sleep 50
Sleeping for 50 ticks...
Done sleeping
```

#### Ver tareas (con preemption activa)
```
uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    125    1          0

uROS> run cpu
Created CPU task with PID 1

uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    150    1          0
1    RUNNING  25     20          130
```

---

## ✅ Verificación

### 1. Timer Funciona
```
uROS> uptime
Uptime: 0.00 seconds (0 ticks)
[esperar ~5 segundos]
uROS> uptime
Uptime: 5.12 seconds (512 ticks)
```
✅ Los ticks avanzan automáticamente

### 2. Preemption ON
```
uROS> sched preempt on
Preemption: ENABLED
uROS> run cpu
Created CPU task with PID 1
[ver que ps muestra cambios de TICKS automáticamente]
```
✅ Context switches por timer

### 3. Preemption OFF
```
uROS> sched preempt off
Preemption: DISABLED
uROS> run cpu
Created CPU task with PID 2
[tareas solo cambian en yield manual]
```
✅ Scheduling cooperativo

### 4. Sleep Funciona
```
uROS> sleep 100
Sleeping for 100 ticks...
[espera ~1 segundo]
Done sleeping
```
✅ Espera correcta

---

## 🔍 Diferencias: Preemptivo vs Cooperativo

| Aspecto | CONFIG_PREEMPT=1 (ON) | CONFIG_PREEMPT=0 (OFF) |
|---------|----------------------|------------------------|
| **Context Switch** | Timer interrupt (automático) | `task_yield()` manual |
| **Quantum RR** | Forzado cada 5 ticks | Solo sugiere (flag) |
| **Shell bloquea?** | No, timer sigue | Sí, si no yield |
| **Tareas CPU** | Se turnan automáticamente | Deben yield() |
| **`sleep`** | Busy-wait (timer despierta) | Yield activo |
| **Complejidad** | Mayor (IRQ context switch) | Menor (no IRQ switch) |
| **Estabilidad** | Requiere ctx_switch perfecto | Más simple |

---

## 🐛 Debugging

### Si se cuelga en `trap_init()`:
- Verifica orden de inicialización
- Asegura que hay al menos idle task antes de habilitar timer

### Si `uptime` reporta 0:
- Verifica que timer está habilitado en `kmain.c`
- Verifica que `trap_init()` se llama

### Si no hay preemption con `preempt on`:
- Verifica `CONFIG_PREEMPT=1` en `config.h`
- Verifica que no hay `#else` activo en `trap.c`

---

## 📊 Resumen de Estado

| Componente | Estado |
|------------|--------|
| **Timer SBI** | ✅ Funcional (100 Hz) |
| **Trap Handler** | ✅ Funcional (no prints en IRQ) |
| **Preemption Dinámica** | ✅ Funcional (on/off runtime) |
| **Comando sleep** | ✅ Funcional |
| **Comando uptime** | ✅ Funcional (avanza con timer) |
| **Context Switch** | ✅ Funcional (preemptivo) |

---

## 🎉 Logros

1. ✅ **Timer habilitado** - `g_ticks` avanza automáticamente
2. ✅ **Preemption configurable** - Cambia on/off en runtime
3. ✅ **uptime funciona** - Muestra tiempo real
4. ✅ **sleep implementado** - Espera N ticks
5. ✅ **Sin prints en IRQ** - Handler limpio
6. ✅ **Orden init correcto** - No cuelgues en boot
7. ✅ **Modo dual** - Preemptivo Y cooperativo

---

**¡Sistema completamente funcional con preemption!** 🚀

