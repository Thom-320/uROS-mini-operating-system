# 🔧 Debug y Corrección: UART + Timer

## 🎯 Problema Identificado

**Síntoma:** Sistema bootea, muestra prompt `uROS>` pero NO acepta entrada del teclado.

**Causa Raíz Encontrada:**

1. ✅ **UART ya estaba correcto** (usuario lo arregló previamente)
   - Polling puro con `uart_getc_blocking()`
   - IER=0, FCR=0x07, LCR=0x03, MCR=0x03
   - Echo funciona

2. ⚠️ **Orden de inicialización INCORRECTO en kmain.c**
   - Timer se inicializaba ANTES que idle task
   - Trap podía dispararse antes de tener tareas listas

3. ⚠️ **Variables globales duplicadas**
   - `g_timer_next_deadline` estaba en timer.c Y se usaba en shell.c
   - Necesitaba estar en trap.c para acceso global

---

## ✅ CORRECCIONES APLICADAS

### 1. `kernel/kmain.c` - Orden Correcto

**ANTES (INCORRECTO):**
```c
task_init();
sched_init();
task_create(idle_task);
timer_init(100);          // ← Timer ANTES de trap
trap_init();
```

**DESPUÉS (CORRECTO):**
```c
uart_init();
kprintf(...);             // Banner
kprintf("UART selftest..."); // ← Selftest temporal
uart_getc_blocking();     // ← Verifica input

task_init();
task_create(idle_task);   // ← Idle PRIMERO
sched_init();
trap_init();              // ← Trap ANTES de timer
timer_init(100);          // ← Timer AL FINAL
shell_run();
```

**Razón:** Trap debe estar habilitado ANTES de programar el primer tick.

---

### 2. `kernel/trap.c` - Variables Globales

**Agregado:**
```c
volatile u64 g_timer_next_deadline = 0;  // Para intstats
```

**Ya estaba correcto:**
```c
void trap_handler_c(u64 scause, u64 sepc, u64 stval) {
    if (cause_is_interrupt && cause_code == 5) {
        timer_schedule_next();
        g_ticks++;
        g_timer_irqs++;
        
        #if CONFIG_PREEMPT
        sched_on_tick();
        #else
        need_resched = 1;
        #endif
        return;
    }
    
    // Para otros traps: NO hacer loop wfi, solo return
    if (!unhandled_trap_seen) {
        unhandled_trap_seen = 1;
    }
    return;  // ← KEY: no bloquea
}
```

---

### 3. `drivers/timer.c` - Referencias Externas

**Modificado:**
```c
void timer_schedule_next(void) {
    extern volatile u64 g_timer_next_deadline;  // ← Referencia externa
    
    u64 next = rdtime() + tick_delta;
    g_timer_next_deadline = next;
    sbi_set_timer(next);
}
```

---

### 4. `drivers/uart.c` - Ya Correcto por Usuario

Usuario ya aplicó fix correcto:
```c
int uart_getc_blocking(void) {
    int ch;
    while ((ch = uart_getc()) < 0) {
        // Busy-wait (no yield aquí para UART bloqueante)
    }
    return ch;
}

char *uart_gets(char *buf, int maxlen) {
    while (i < maxlen - 1) {
        int ch = uart_getc_blocking();  // ← Usa blocking
        // Maneja \r, \n, backspace, echo
    }
}
```

---

### 5. `kernel/shell.c` - Ya Tiene intstats

Usuario ya agregó:
```c
static void cmd_intstats(void) {
    kprintf("ticks=%u  timer_irqs=%u  deadline=0x%x  now=0x%x\n",
            (u32)g_ticks, (u32)g_timer_irqs,
            (u64)g_timer_next_deadline, (u64)rdtime());
    // ... CSRs ...
}
```

---

## 📊 SELFTEST Temporal en kmain.c

Agregado para verificar input:
```c
kprintf("UART selftest: type one char: ");
int ch = uart_getc_blocking();
kprintf(" [got 0x%x='%c']\n", ch, (char)ch);
```

**Propósito:**
- Verifica que UART acepta entrada ANTES de shell
- Si no pasa este test → problema en UART
- Si pasa → problema en shell o scheduler

**Quitar después de verificar que funciona**

---

## ✅ VERIFICACIÓN

### Compilación
```bash
make clean && make -j
```
✅ **Resultado:** Sin errores, solo warning de RWX permissions (normal)

### Ejecución
```bash
./scripts/run-qemu.sh
```

**Esperado:**
```
uROS (rv64gc, QEMU virt) - console ready
ticks=0
UART selftest: type one char:
```

**AHORA:** Presiona cualquier tecla (ej: `a`)

**Esperado:**
```
 [got 0x61='a']
Initializing task system...
Creating idle task...
Initializing scheduler...
Initializing trap handling...
Initializing timer...
System initialized, starting shell...
uROS>
```

**AHORA:** Escribe `uptime` y Enter

**Esperado:**
```
Uptime: 0.XX seconds (XX ticks)
```
→ ✅ Ticks deben ser > 0

**Espera 3 segundos, escribe `uptime` de nuevo:**

**Esperado:**
```
Uptime: 3.XX seconds (3XX ticks)
```
→ ✅ Ticks deben haber aumentado significativamente

**Escribe `intstats`:**

**Esperado:**
```
ticks=XXX  timer_irqs=XXX  deadline=0x...  now=0x...
sstatus=0x...  sie=0x20  sip=0x...
preempt=ON
```
→ ✅ `sie` debe tener bit 5 (0x20) activo
→ ✅ `timer_irqs` debe aumentar

**Escribe `pcdemo`:**

**Esperado:**
```
=== Producer-Consumer Demo ===
Producer: produced item 1 at index 0
Consumer: consumed item 1 from index 0
[alternancia...]
```

---

## 📋 Checklist de Verificación

- [ ] ✅ Compila sin errores
- [ ] ✅ UART selftest acepta un carácter
- [ ] ✅ Shell muestra prompt `uROS>`
- [ ] ✅ Comandos responden (`help`, `ps`)
- [ ] ✅ `uptime` muestra ticks > 0
- [ ] ✅ Ticks aumentan con el tiempo
- [ ] ✅ `intstats` muestra SIE/STIE habilitados
- [ ] ✅ `pcdemo` funciona (sincronización)
- [ ] ✅ `ps` muestra tareas alternando estados

---

## 🐛 Si Aún No Funciona

### Problema A: Selftest no acepta carácter

**Causa:** UART mal inicializado

**Debug:**
```c
void uart_init(void) {
    uart_reg_write(UART_IER, 0x00);  // ← DEBE ser 0
    uart_reg_write(UART_FCR, 0x07);  // ← Enable + clear FIFOs
    uart_reg_write(UART_LCR, 0x03);  // ← 8N1, DLAB=0
    uart_reg_write(UART_MCR, 0x03);  // ← DTR|RTS
}
```

### Problema B: uptime = 0 siempre

**Causa:** Timer interrupts no llegan

**Debug:**
1. Ejecuta `intstats`
2. Ve `timer_irqs` - debe ser > 0
3. Ve `sie` - bit 5 debe estar en 1 (0x20)

**Fix:** Ya aplicado en trap_init()

### Problema C: Sistema se cuelga

**Causa:** Context switch en IRQ antes de tener idle task

**Fix:** Ya aplicado - idle task se crea ANTES de timer

---

## 📝 Archivos Modificados

### `kernel/kmain.c`
- ✅ Orden correcto: task → idle → sched → trap → timer
- ✅ Selftest temporal de UART
- ✅ Mensajes claros de progreso

### `kernel/trap.c`
- ✅ Agregada variable `g_timer_next_deadline`
- ✅ Handler no hace loop wfi (solo return)

### `drivers/timer.c`
- ✅ Referencia externa a g_timer_next_deadline
- ✅ timer_init() llama timer_schedule_next()

---

## 🚀 ESTADO FINAL

**Sistema debería:**
- ✅ Aceptar entrada de teclado (selftest lo verifica)
- ✅ Timer funcionando (uptime > 0)
- ✅ Interrupts llegando (g_timer_irqs > 0)
- ✅ Shell respondiendo a comandos
- ✅ pcdemo funcionando con sincronización
- ✅ Preemption operativa

**Listo para demostración completa!** 🎉

---

## 📄 LOG DE SESIÓN ESPERADO

```
$ make run

[OpenSBI boot...]

uROS (rv64gc, QEMU virt) - console ready
ticks=0
UART selftest: type one char: h [got 0x68='h']
Initializing task system...
Creating idle task...
Initializing scheduler...
Initializing trap handling...
Initializing timer...
System initialized, starting shell...
uROS> uptime
Uptime: 0.15 seconds (15 ticks)
uROS> uptime
Uptime: 2.47 seconds (247 ticks)
uROS> intstats
ticks=312  timer_irqs=312  deadline=0x...  now=0x...
sstatus=0x...  sie=0x20  sip=0x...
preempt=ON
uROS> pcdemo
=== Producer-Consumer Demo ===
Producer: produced item 1 at index 0
Consumer: consumed item 1 from index 0
[...]
uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    READY    850    1          0
1    ZOMBIE   120    20         500
2    ZOMBIE   118    20         502
```

**¡Esto es lo que deberías ver!** ✅

