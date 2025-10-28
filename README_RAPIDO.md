# uROS - Guía Rápida 🚀

## ¿Qué es uROS?

uROS es un mini sistema operativo para RISC-V 64 que corre en QEMU con:
- ✅ Shell interactiva
- ✅ Scheduler Round-Robin
- ✅ Sistema de tareas/threads
- ✅ Comandos de gestión

## Inicio Rápido (3 pasos)

### 1️⃣ Compilar
```bash
make clean && make -j
```

### 2️⃣ Ejecutar
```bash
make run
```

### 3️⃣ Probar Comandos
En el prompt `uROS>`, escribe:
```
help
ps
run cpu
run io
ps
sched rr
meminfo
```

**Para salir**: Presiona `Ctrl+C`

## Demostración Automática

```bash
./scripts/demo.sh
```

## Comandos Disponibles

| Comando | Descripción |
|---------|-------------|
| `help` | Muestra ayuda |
| `ps` | Lista tareas/procesos |
| `run cpu` | Crea tarea CPU-bound |
| `run io` | Crea tarea I/O-bound |
| `kill <pid>` | Termina una tarea |
| `sched rr` | Cambia a Round-Robin |
| `sched sjf` | Cambia a SJF (requiere timer) |
| `bench` | Ejecuta benchmark (requiere timer) |
| `uptime` | Tiempo de ejecución |
| `meminfo` | Uso de memoria |

## Verificación

Para verificar que todo funciona:
```bash
# Test rápido
echo "help" | timeout 5 ./scripts/run-qemu.sh

# Test completo
echo -e "help\nps\nrun cpu\nrun io\nps\nmeminfo" | timeout 10 ./scripts/run-qemu.sh
```

## Estado del Proyecto

✅ **FUNCIONAL** - Listo para demostración

**Características implementadas:**
- UART (consola)
- Printf
- Shell interactiva
- Tareas/threads
- Scheduler Round-Robin cooperativo
- Todos los comandos básicos

**Limitaciones actuales:**
- Timer deshabilitado por estabilidad
- `uptime` reporta 0
- `bench` no puede medir tiempos sin timer
- `sched sjf` no funcional sin timer

## Archivos Importantes

- `kernel/kmain.c` - Punto de entrada
- `kernel/shell.c` - Shell interactiva
- `kernel/sched.c` - Scheduler
- `kernel/task.c` - Sistema de tareas
- `drivers/uart.c` - Driver de consola
- `scripts/run-qemu.sh` - Lanzador QEMU

## Debugging

Para ejecutar con GDB:
```bash
make run-gdb    # En una terminal
make gdb        # En otra terminal
```

## Soporte

Ver documentación completa en:
- `VERIFICATION.md` - Verificación detallada
- `RESUMEN_VERIFICACION.md` - Resumen completo
- `docs/README.md` - Documentación técnica

---

**¡Listo para usar!** 🎉

