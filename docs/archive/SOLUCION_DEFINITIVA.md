# 🎯 SOLUCIÓN DEFINITIVA

## Problema Actual

Shell muestra `uROS>` pero no acepta entrada cuando timer está activo.

## Causa Real

`uart_getc_blocking()` hace busy-wait infinito sin yield:
```c
int uart_getc_blocking(void) {
    int ch;
    while ((ch = uart_getc()) < 0) {
        // ← Aquí se queda en loop infinito
        // Timer dispara, hace context switch
        // Pero no hay otro sitio donde volver
    }
    return ch;
}
```

Cuando el timer interrumpe DENTRO de este busy-wait y hace context switch a idle task, el sistema se bloquea porque:
1. Shell está en medio de `uart_getc_blocking()`
2. Timer hace context switch a idle
3. Idle hace `wfi` (espera interrupción)
4. Siguiente timer vuelve a idle
5. Nunca vuelve a shell
6. **Loop infinito idle ↔ timer**

## Solución Simple

Agregar yield en `uart_getc_blocking()`:

```c
int uart_getc_blocking(void) {
    int ch;
    while ((ch = uart_getc()) < 0) {
        // Yield para que otras tareas puedan correr
        if (sched_current()) {
            task_yield();
        }
    }
    return ch;
}
```

O mejor: deshabilitar preemption mientras espera input:

```c
char *uart_gets(char *buf, int maxlen) {
    // Deshabilitar interrupts durante input
    disable_irq();
    
    int i = 0;
    while (i < maxlen - 1) {
        // Re-habilitar momentáneamente para permitir timer
        enable_irq();
        __asm__ volatile("nop");
        disable_irq();
        
        int ch = uart_getc();
        if (ch == -1) continue;
        
        // ... manejo de ch ...
    }
    
    enable_irq();
    return buf;
}
```


