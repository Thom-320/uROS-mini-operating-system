# 🆘 PROBLEMA TÉCNICO - Necesitamos Ayuda de Codex

## 🎯 DESCRIPCIÓN DEL PROBLEMA

**Sistema:** uROS mini-OS en RISC-V 64 (rv64gc) sobre QEMU virt con OpenSBI

**Síntoma:** 
- Sistema bootea correctamente
- Muestra prompt `uROS>`
- **NO acepta entrada del teclado** (no responde a ninguna tecla)

**Contexto:**
- UART NS16550A @ 0x10000000 (polling, IER=0, sin interrupciones UART)
- Timer SBI @ 100Hz funcionando
- Shell llama `uart_gets()` que llama `uart_getc_blocking()`

---

## 🧪 TESTS REALIZADOS

| Test | Resultado |
|------|-----------|
| TEST 1: Solo UART echo | ✅ FUNCIONA - acepta input |
| TEST 2: Shell simple sin timer | ✅ FUNCIONA - acepta comandos |
| TEST 3: Sistema sin timer | ✅ FUNCIONA - shell responde |
| TEST 4: Con timer + trap | ❌ NO RESPONDE - se bloquea |
| TEST 5: Con timer sin trap | ✅ FUNCIONA |

**Conclusión:** El problema ocurre cuando `trap_init()` + `timer_init()` están activos.

---

## 📋 CONFIGURACIÓN ACTUAL

### `include/config.h`
```c
#define CONFIG_PREEMPT 0  // Modo cooperativo
```

### `kernel/trap.c`
```c
void trap_init(void) {
    csrw stvec, handler;
    sbi_set_timer(~0ULL);  // Máscara timer antes de habilitar
    csrsi sstatus, 0x2;    // Habilita SIE
    csrs sie, 0x20;        // Habilita STIE
}

void trap_handler_c(u64 scause, ...) {
    if (is_s_timer_interrupt(scause)) {
        timer_schedule_next();
        g_ticks++;
        sched_on_tick();     // Modo cooperativo: no hace yield
        need_resched = 1;
        return;
    }
    return;  // Otros traps: solo return
}
```

### `drivers/uart.c`
```c
int uart_getc_blocking(void) {
    int ch;
    while ((ch = uart_getc()) < 0) {
        __asm__ volatile("wfi");  // Espera interrupción
    }
    return ch;
}

char *uart_gets(char *buf, int maxlen) {
    while (i < maxlen - 1) {
        int ch = uart_getc_blocking();  // Se queda aquí
        // ... manejo de ch ...
    }
}
```

### `kernel/kmain.c`
```c
void kmain(void) {
    uart_init();
    kprintf("uROS ready\n");
    
    task_init();
    task_create(idle_task, 0, 1);
    sched_init();
    trap_init();      // Habilita interrupts
    timer_init();     // Programa primer tick
    
    shell_run();      // Llama uart_gets() → uart_getc_blocking()
}
```

---

## 🤔 HIPÓTESIS DEL PROBLEMA

### Hipótesis A: Context Switch Involuntario
Aunque `CONFIG_PREEMPT=0`, `sched_on_tick()` puede estar haciendo algo que corrompe el estado de la shell.

### Hipótesis B: WFI Problemático
El `wfi` en `uart_getc_blocking()` duerme el CPU, timer despierta, pero algo en el flujo de retorno está roto.

### Hipótesis C: No Hay Current Task
La shell NO está corriendo como una tarea, es parte de kmain. Cuando timer dispara y llama `sched_on_tick()`, puede intentar operar sobre `current_task = NULL`.

### Hipótesis D: UART Perdió Configuración
Algo en el flujo de interrupciones está corrompiendo los registros UART (IER cambia, DLAB se activa, etc).

---

## 🔍 INFORMACIÓN DE DEBUG

### Lo Que Sabemos

1. ✅ UART funciona (TEST 1 probado)
2. ✅ Shell funciona sin timer (TEST 2, 3 probados)
3. ❌ Con timer + trap se bloquea (TEST 4)
4. ✅ Con timer pero sin habilitar interrupts funciona (TEST 5)

### Estado al Bloquearse

```
System initialized, starting shell...
uROS>
[cursor aquí - no acepta teclas]
```

**La shell está en:** `uart_gets()` → `uart_getc_blocking()` → loop con `wfi`

**Timer:** Disparando cada 10ms, incrementando g_ticks

**Scheduler:** `current_task = NULL` (shell no es una tarea)

---

## 💡 SOLUCIONES POSIBLES

### Opción 1: NO llamar sched_on_tick() si current_task == NULL

```c
void trap_handler_c(...) {
    if (is_s_timer_interrupt(scause)) {
        timer_schedule_next();
        g_ticks++;
        
        // Solo actualizar scheduler si hay tarea actual
        extern pcb_t *sched_current(void);
        if (sched_current()) {
            sched_on_tick();
        }
        
        need_resched = 1;
        return;
    }
}
```

### Opción 2: Shell como Tarea

Hacer que `shell_run()` corra como una tarea en lugar de directamente desde kmain.

### Opción 3: Deshabilitar Interrupts Durante uart_gets

```c
char *uart_gets(char *buf, int maxlen) {
    disable_irq();  // No interrupciones durante input
    
    // ... código existente ...
    
    enable_irq();
    return buf;
}
```

### Opción 4: Eliminar WFI de uart_getc_blocking

```c
int uart_getc_blocking(void) {
    int ch;
    while ((ch = uart_getc()) < 0) {
        // Busy-wait simple sin wfi
        __asm__ volatile("nop");
    }
    return ch;
}
```

---

## 🆘 PREGUNTA PARA CODEX

**ChatGPT Codex:**

Mi sistema uROS (RISC-V en QEMU) bootea y muestra `uROS>` pero NO acepta entrada del teclado cuando timer está activo.

**Configuración:**
- UART polling (sin interrupciones UART)
- Timer SBI @ 100Hz con interrupciones habilitadas
- CONFIG_PREEMPT=0 (cooperativo)
- Shell NO es una tarea (corre desde kmain directamente)
- `current_task = NULL` cuando shell espera input

**Tests:**
- Sin timer: ✅ Shell funciona
- Con timer: ❌ Se bloquea en `uart_getc_blocking()`

**Código crítico:**
```c
// Shell espera aquí:
int uart_getc_blocking(void) {
    while ((ch = uart_getc()) < 0) {
        wfi;  // ← Se queda aquí
    }
}

// Timer dispara esto cada 10ms:
void trap_handler_c(...) {
    if (s-timer) {
        g_ticks++;
        sched_on_tick();  // ← current_task == NULL
        return;
    }
}
```

**Pregunta:** ¿Cómo hacer que shell acepte input mientras timer está activo y current_task == NULL?

**Opciones consideradas:**
1. No llamar `sched_on_tick()` si `current_task == NULL`
2. Deshabilitar IRQ durante `uart_gets()`
3. Quitar `wfi` de `uart_getc_blocking()`
4. Hacer shell una tarea

¿Cuál es la mejor solución?

---

## 📝 ARCHIVOS CLAVE

- `kernel/kmain.c` - Inicialización
- `kernel/trap.c` - Trap handler
- `drivers/uart.c` - uart_getc_blocking()
- `kernel/sched.c` - sched_on_tick()
- `kernel/shell.c` - shell_run()

---

**Por favor, ayúdanos Codex.** 🙏

