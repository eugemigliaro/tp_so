# TP2 Sistemas Operativos (72.11)

## Integrantes

| Nombre                 | Legajo | Mail                       |
|------------------------|--------|----------------------------|
| Valentín Ruiz          | 64046  | vruiz@itba.edu.ar          |
| Santiago Allegretti    | 65531  | sallegretti@itba.edu.ar    |
| Eugenio Migliaro       | 65508  | emigliaro@itba.edu.ar      |

---

## Instrucciones de Compilación y Ejecución

### Requisitos Previos
- [Docker](https://docker.com/products/docker-desktop/) instalado y en ejecución
- Imagen de Docker: `agodio/itba-so-multi-platform:3.0`

### Compilación

El proyecto soporta dos administradores de memoria que pueden ser seleccionados en tiempo de compilación:

#### Compilar con Buddy System (por defecto)
```bash
./compile.sh
```
o explícitamente:
```bash
./compile.sh buddy
```

#### Compilar con Memory Manager Personalizado (myMalloc)
```bash
./compile.sh mymalloc
```

### Ejecución

```bash
./run.sh
```

### Compilación y Ejecución Rápida
```bash
./compile.sh && ./run.sh
```

---

## Instrucciones de Replicación

### Shell y Comandos Especiales

#### Caracteres Especiales
- **`|` (pipe)**: Conecta la salida de un comando con la entrada de otro
  - Ejemplo: `cat | wc`
  - Limitación: Solo soporta 2 comandos conectados
- **`&` (background)**: Ejecuta un comando en segundo plano
  - Ejemplo: `loop 5 &`

#### Atajos de Teclado
- **Ctrl + C**: Mata el proceso en foreground
- **Ctrl + D**: Envía End Of File (EOF)
- **Flechas ↑/↓**: Navega por el historial de comandos

### Comandos Disponibles

#### Comandos Generales
- **`help`**: Muestra la lista de todos los comandos disponibles
- **`clear`**: Limpia la pantalla
- **`time`**: Muestra la hora actual del sistema
- **`history`**: Muestra el historial de comandos ejecutados
- **`man <comando>`**: Muestra la descripción de un comando
- **`exit`**: Sale de la shell
- **`font <increase|decrease>`**: Aumenta o disminuye el tamaño de fuente

#### Comandos de Excepción (Testing)
- **`divzero`**: Genera una excepción de división por cero
- **`invop`**: Genera una excepción de operación inválida
- **`regs`**: Muestra el snapshot de registros del procesador

#### Physical Memory Management
- **`mem`**: Imprime el estado de la memoria (total, ocupada, libre)

#### Gestión de Procesos
- **`ps`**: Lista todos los procesos con sus propiedades
  - Muestra: PID, nombre, prioridad, stack pointer, base pointer, estado, foreground/background
- **`loop [segundos]`**: Imprime su PID periódicamente cada N segundos (por defecto: 3)
  - Ejemplo: `loop 5`
- **`kill <pid>`**: Mata un proceso dado su PID
  - Ejemplo: `kill 3`
- **`nice <pid> <prioridad>`**: Cambia la prioridad de un proceso
  - Ejemplo: `nice 3 0` (prioridad máxima) o `nice 3 5` (prioridad mínima)
  - Rango de prioridad: 0-5 (menor número = mayor prioridad, 0 es la más alta)
- **`block <pid>`**: Alterna entre bloquear y desbloquear un proceso
  - Ejemplo: `block 3`

#### Inter Process Communication
- **`cat`**: Lee de stdin y escribe a stdout tal como lo recibe
  - Útil para pipes
  - Ejemplo: `cat` (modo interactivo) o `filter | cat`
- **`wc`**: Cuenta la cantidad de líneas del input
  - Ejemplo: `cat | wc`
- **`filter`**: Filtra las vocales del input
  - Ejemplo: `cat | filter`
- **`mvar <escritores> <lectores>`**: Problema de múltiples lectores y escritores
  - Implementa sincronización estilo MVar (Haskell)
  - Ejemplo: `mvar 2 3`
  - Crea N procesos escritores y M procesos lectores que acceden a una variable compartida
  - Cada escritor escribe un carácter único (A, B, C, etc.)
  - Cada lector consume e imprime el valor junto con su identificador (1, 2, 3, etc.)
  - El comando termina inmediatamente, dejando el coordinador en background
  - Para detener: `kill <pid_coordinador>` (el PID se muestra al ejecutar)

### Tests Provistos por la Cátedra

Todos los tests pueden ejecutarse en foreground o background (añadiendo `&`).

#### `tmm <max_memory_bytes>`
- **Descripción**: Test de stress del memory manager
- **Funcionamiento**: Ciclo infinito que pide y libera bloques de memoria de tamaño aleatorio, verificando que no se solapen
- **Parámetro**: Cantidad máxima de memoria a utilizar en bytes
- **Ejemplo**: 
  ```bash
  tmm 10000000
  tmm 5000000 &
  ```
- **Para detener**: Ctrl + C (si está en foreground) o `kill <pid>`

#### `tproc <max_processes>`
- **Descripción**: Test de ciclo de vida de procesos
- **Funcionamiento**: Crea, bloquea, desbloquea y mata procesos dummy aleatoriamente
- **Parámetro**: Cantidad máxima de procesos a crear
- **Ejemplo**: 
  ```bash
  tproc 10
  tproc 5 &
  ```

#### `tprio <max_value>`
- **Descripción**: Test de prioridades del scheduler
- **Funcionamiento**: Crea 3 procesos que incrementan variables. Primero con igual prioridad, luego con prioridades diferentes
- **Parámetro**: Valor al que deben llegar las variables para finalizar
- **Ejemplo**: 
  ```bash
  tprio 1000000
  ```

#### `tsem <num_processes> <num_operations>`
- **Descripción**: Test de sincronización con semáforos
- **Funcionamiento**: Crea procesos que incrementan y decrementan una variable compartida usando semáforos. El resultado final debe ser 0
- **Parámetros**: 
  - Número de procesos
  - Número de incrementos/decrementos por proceso
- **Ejemplo**: 
  ```bash
  tsem 5 1000
  ```

### Ejemplos de Uso

#### Memory Management
```bash
# Ver estado de la memoria
mem

# Ejecutar test de memoria en background
tmm 10000000 &

# Ver procesos y memoria mientras el test corre
ps
mem
```

#### Procesos y Scheduling
```bash
# Crear varios procesos loop en background
loop 2 &
loop 3 &
loop 5 &

# Listar procesos
ps

# Cambiar prioridad de un proceso
nice 3 10

# Bloquear un proceso
block 4

# Desbloquear el mismo proceso
block 4

# Matar un proceso
kill 3
```

#### Pipes e IPC
```bash
# Filtrar vocales y contar líneas
filter | wc

# Usar cat como intermediario
cat | filter

# Cadena de pipes simple
cat | wc
```

#### Problema MVar - Múltiples Lectores/Escritores
```bash
# 2 escritores, 2 lectores - ejecución balanceada
mvar 2 2

# 3 escritores, 2 lectores - más escrituras
mvar 3 2

# 2 escritores, 3 lectores - más lecturas
mvar 2 3

# Ejecutar en background y manipular
mvar 2 2 &
# Obtener PID con ps
ps
# Matar un escritor para observar comportamiento
kill <pid_escritor>
# Cambiar prioridad de un escritor
nice <pid_escritor> 10
```

#### Tests de Sincronización
```bash
# Test con semáforos (resultado debe ser 0)
tsem 5 10000

# Test de prioridades
tprio 1000000

# Test de procesos en background
tproc 8 &
```

#### Combinación de Comandos
```bash
# Múltiples procesos con diferentes configuraciones
loop 2 &
loop 4 &
mvar 2 2 &
ps
nice 3 8
block 4
ps
```

---

## Requerimientos Implementados

### ✅ Physical Memory Management
- [x] Memory Manager personalizado (myMalloc) con liberación de memoria
- [x] Buddy System
- [x] Selección en tiempo de compilación
- [x] Interfaz común intercambiable
- [x] Syscalls: malloc, free, mem_status

### ✅ Procesos, Context Switching y Scheduling
- [x] Multitasking preemptivo
- [x] Round Robin con prioridades (0-5)
- [x] Syscalls: create, exit, getpid, ps, kill, nice, block/unblock, yield, waitpid

### ✅ Sincronización
- [x] Semáforos con identificador compartido
- [x] Sin busy waiting, deadlock o race conditions
- [x] Instrucciones atómicas
- [x] Syscalls: sem_open, sem_close, sem_wait, sem_post

### ✅ Inter Process Communication
- [x] Pipes unidireccionales
- [x] Operaciones bloqueantes de lectura/escritura
- [x] Transparencia entre pipes y terminal
- [x] Compartición por identificador
- [x] Integración con shell (operador `|`)

### ✅ Aplicaciones de User Space
- [x] Shell (sh) con pipes y background (`|`, `&`)
- [x] Soporte Ctrl+C y Ctrl+D
- [x] help, mem, ps, loop, kill, nice, block
- [x] cat, wc, filter, mvar
- [x] Tests: tmm, tproc, tprio, tsem

---

## Limitaciones

### Pipes
- **Cantidad máxima**: 100 pipes simultáneos (`MAX_PIPES = 100`)
- **Tamaño del buffer**: 1024 bytes por pipe (`PIPE_BUFFER_SIZE = 1024`)
- Solo soporta conexión de 2 comandos (no admite `p1 | p2 | p3`)
- Los pipes solo funcionan con comandos externos (no built-ins de la shell)
- No soporta EOF explícito en pipes (Ctrl+D solo funciona en stdin de terminal)

### Procesos
- **Número máximo**: 32 procesos simultáneos (`PROCESS_MAX_PROCESSES = 32`)
- **Tamaño de stack**: 16 KB por proceso (`PROCESS_STACK_SIZE = 16384` bytes)
- **Máximo de argumentos**: 64 argumentos por comando (`MAX_ARGS = 64`)
- **Tamaño máximo de argumento**: 256 caracteres (`MAX_ARGUMENT_SIZE = 256`)
- Los procesos en background no pueden ser traídos a foreground posteriormente
- **Memory leaks al matar procesos**: Cuando se mata un proceso con `kill`, no se liberan automáticamente las allocations de memoria dinámica que haya realizado (solo se libera el stack). Se asume que los procesos son bien comportados y liberan su memoria antes de terminar. Esta es una limitación conocida del sistema.
- No hay límite explícito en el nombre del proceso (usa argv[0]), pero argv[0] está limitado por `MAX_ARGUMENT_SIZE`

### Scheduling
- **Niveles de prioridad**: 6 niveles (0-5, donde 0 es mayor prioridad)
- **Quantum por defecto**: 4 ticks del timer
- **Aging threshold**: 32 ticks en estado ready antes de aumentar prioridad temporalmente

### Semáforos
- No hay límite explícito en la cantidad de semáforos (limitado solo por memoria disponible)
- **Tamaño de nombre**: Sin límite explícito, pero los semáforos internos de pipes usan 8 caracteres
- Los semáforos no se destruyen automáticamente cuando no hay procesos esperando

### Memory Manager
- Solo un memory manager activo por compilación (no intercambiable en runtime)
- La memoria total disponible es fija y definida en tiempo de boot

### Shell
- **Buffer de entrada**: 1024 caracteres (`MAX_BUFFER_SIZE = 1024`)
- **Historial**: 10 comandos (`HISTORY_SIZE = 10`)
- No soporta redirección a archivos (`>`, `<`, `>>`)
- No soporta variables de entorno
- No soporta expansión de wildcards (`*`, `?`)
- No soporta job control completo (no se puede suspender con Ctrl+Z ni traer procesos de background a foreground)

---

## Citas de Fragmentos de Código / Uso de IA

### Uso de Material de Referencia
- **Base del proyecto**: BareBones x86-64 de Arquitectura de Computadoras (ITBA)
- **Bootloader**: Pure64 (retrowavesoft/pure64) y BMFS
- **Drivers**: Adaptados del TP de Arquitectura de Computadoras (teclado, video, RTC)

### Consultas con IA
- Se utilizó GitHub Copilot para autocompletado y sugerencias de código
- Todo el código generado fue revisado, comprendido y adaptado por el equipo

### Código de Terceros
- **Buddy System**: Implementación basada en la documentación de FreeRTOS Memory Management
  - Referencia: https://freertos.org/Documentation/02-Kernel/02-Kernel-features/09-Memory-management/01-Memory-management
  - Adaptado y modificado para arquitectura x86-64
- **Semáforos**: Implementación inspirada en ejemplos de sistemas POSIX

---

## Notas Adicionales

- El sistema está libre de warnings con `-Wall` activado
- Compatible con análisis de PVS-Studio
- Control de versiones desde el inicio del desarrollo
- Todos los tests provistos por la cátedra ejecutan correctamente
