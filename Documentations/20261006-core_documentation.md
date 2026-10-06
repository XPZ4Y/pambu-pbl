# PAMBU CPU Core – `core.c` Documentation

**Author of the code:** Falah
**Source:** `BaseSystem/src/core.c` ([GitHub](https://github.com/XPZ4Y/pambu-pbl/blob/main/BaseSystem/src/core.c))
**Document date:** 2026-10-06

---

## 1. Overview

`core.c` is the **CPU core** of the Pambu base system. It simulates a very small CPU that owns:

- a **memory** array (100 integer cells),
- a **stack** (LIFO, 100 entries),
- a **queue** (FIFO, 100 entries),
- basic **arithmetic** (add, subtract).

The core does not talk to the user directly. It runs as its own process and communicates with the other components (UI and Logger) through **POSIX message queues** (`<mqueue.h>`):

```
 ┌──────────┐  /pambu_request   ┌──────────────┐  /pambu_response  ┌──────────┐
 │    UI    │ ────────────────▶ │  CPU CORE    │ ────────────────▶ │    UI    │
 └──────────┘                   │  (core.c)    │                   └──────────┘
                                └──────┬───────┘
                                       │ /pambu_log
                                       ▼
                                 ┌──────────┐
                                 │  Logger  │
                                 └──────────┘
```

The core receives a text command, executes it, sends a result back to the UI, and sends a log line to the Logger.

---

## 2. Building and Running

Message queues on Linux require the real-time library:

```bash
gcc core.c -o core -lrt
```

**Start order matters:**

1. Start the **Logger** first (it must create `/pambu_log`).
2. Start the **Core** (`./core`). It creates `/pambu_request` and `/pambu_response`, and connects to `/pambu_log`.
3. Start the **UI**, which sends commands to `/pambu_request` and reads from `/pambu_response`.

If the Logger is not running, the core prints `CORE: Cannot connect to logger`, cleans up and exits with code `1`.

---

## 3. Constants and Global State

| Name | Value | Purpose |
|---|---|---|
| `REQUEST_QUEUE` | `/pambu_request` | UI → Core commands |
| `RESPONSE_QUEUE` | `/pambu_response` | Core → UI results |
| `LOG_QUEUE` | `/pambu_log` | Core → Logger entries |
| `MAX_MESSAGE` | 256 | Max message size in bytes (also queue message size) |
| `STACK_SIZE` | 100 | Stack capacity |
| `QUEUE_SIZE` | 100 | CPU queue capacity |
| `MEMORY_SIZE` | 100 | Number of memory cells |

| Variable | Description |
|---|---|
| `int memory[MEMORY_SIZE]` | Simulated RAM. Global, so zero-initialised. |
| `int stack[STACK_SIZE]` / `stack_top` | Stack storage and index of the top element (`-1` = empty). |
| `int cpu_queue[QUEUE_SIZE]` / `queue_front` / `queue_rear` | Queue storage, index of the front element (starts `0`) and of the last element (starts `-1`). |

---

## 4. CPU Primitives

### 4.1 Stack

| Function | Behaviour |
|---|---|
| `void cpu_push(int value)` | If `stack_top >= STACK_SIZE - 1`, prints `CORE: Stack overflow` and returns. Otherwise stores the value at `stack[++stack_top]`. |
| `int cpu_pop(void)` | If `stack_top < 0`, prints `CORE: Stack underflow` and returns `-1`. Otherwise returns `stack[stack_top--]`. |

### 4.2 Queue

| Function | Behaviour |
|---|---|
| `void cpu_enqueue(int value)` | If `queue_rear >= QUEUE_SIZE - 1`, prints `CORE: Queue full` and returns. Otherwise stores at `cpu_queue[++queue_rear]`. |
| `int cpu_dequeue(void)` | If `queue_front > queue_rear`, prints `CORE: Queue empty` and returns `-1`. Otherwise returns `cpu_queue[queue_front++]`. |

### 4.3 Arithmetic

| Function | Returns |
|---|---|
| `int cpu_add(int a, int b)` | `a + b` |
| `int cpu_subtract(int a, int b)` | `a - b` |

---

## 5. Communication Helpers

```c
void send_response(mqd_t response_queue, const char *message);
void send_log(mqd_t log_queue, const char *message);
```

Both wrap `mq_send()` with priority `0`, sending `strlen(message) + 1` bytes (so the null terminator is included). If sending fails, they print an error with `perror` but the core keeps running.

---

## 6. Command Reference

Commands are plain text strings sent to `/pambu_request`. `process_command()` tries each pattern in order using `sscanf` (or `strcmp` for commands without arguments).

| Command | Syntax | Response to UI | Log message |
|---|---|---|---|
| ADD | `ADD <a> <b>` | `ADD result = <r>` | `ADD <a> <b> = <r>` |
| SUBTRACT | `SUBTRACT <a> <b>` | `SUBTRACT result = <r>` | `SUBTRACT <a> <b> = <r>` |
| PUSH | `PUSH <value>` | `Value pushed to stack` | `PUSH <value>` |
| POP | `POP` | `POP result = <r>` | `POP -> <r>` |
| ENQUEUE | `ENQUEUE <value>` | `Value added to queue` | `ENQUEUE <value>` |
| DEQUEUE | `DEQUEUE` | `DEQUEUE result = <r>` | `DEQUEUE -> <r>` |
| STORE | `STORE <address> <value>` | `Value stored in memory` | `STORE memory[<a>] = <v>` |
| LOAD | `LOAD <address>` | `Memory[<a>] = <v>` | `LOAD memory[<a>] -> <v>` |
| exit | `exit` | `Core shutting down` | `Core shutting down` |

**Error cases**

- `STORE` / `LOAD` with an address outside `0..99` → response and log both `ERROR: Invalid memory address`.
- Any unrecognised command → response `ERROR: Unknown command`; log `ERROR: Unknown command -> <command>` (command truncated to 200 characters so the log line fits in `MAX_MESSAGE`).

**Examples**

```
ADD 5 3          → ADD result = 8
PUSH 42          → Value pushed to stack
POP              → POP result = 42
STORE 10 99      → Value stored in memory
LOAD 10          → Memory[10] = 99
LOAD 500         → ERROR: Invalid memory address
HELLO            → ERROR: Unknown command
```

---

## 7. Program Flow (`main`)

### 7.1 Startup

1. Fill a `struct mq_attr` with `mq_maxmsg = 10` and `mq_msgsize = MAX_MESSAGE`.
2. `mq_unlink()` the request and response queues, removing stale queues left by a previous crashed run.
3. Create `/pambu_request` (`O_CREAT | O_RDONLY`, mode `0666`).
4. Create `/pambu_response` (`O_CREAT | O_WRONLY`, mode `0666`).
5. Open the existing `/pambu_log` (`O_WRONLY`), created by the Logger.
6. Print the "PAMBU CPU CORE" banner showing CPU, Memory, Stack, Queue and Logger status.

Each step closes and unlinks whatever was already created before returning `1` if it fails.

### 7.2 Main loop

```
while (1):
    received = mq_receive(request_queue, command, MAX_MESSAGE)   # blocks
    if received == -1: perror, break
    command[received] = '\0'
    print "CORE received: <command>"
    if command == "exit": send response + log, break
    process_command(command, response_queue, log_queue)
```

`mq_receive` blocks, so the core uses no CPU while idle.

### 7.3 Shutdown

On `exit` (or a receive error) the core closes all three queue descriptors, unlinks the request and response queues, prints `CORE: Shutdown complete.` and returns `0`. The log queue is **not** unlinked because the Logger owns it.

---

## 8. Known Limitations and Suggested Improvements

These are observations from reading the code, not necessarily bugs for the current project scope.

1. **Linear (non-circular) queue.** `queue_front` and `queue_rear` only move forward. After 100 total enqueues the queue reports "full" even if every item was dequeued. A circular buffer (`rear = (rear + 1) % QUEUE_SIZE` with a count) would fix this.
2. **Success message on failure.** `PUSH` and `ENQUEUE` always reply "Value pushed to stack" / "Value added to queue", even when the stack overflowed or the queue was full. Making `cpu_push` / `cpu_enqueue` return a status code would allow an error response.
3. **Ambiguous `-1`.** `POP` and `DEQUEUE` return `-1` on error, which is also a valid stored value. A separate status/out-parameter would remove the ambiguity.
4. **Possible buffer overrun.** `command[received] = '\0'` writes out of bounds if `received == MAX_MESSAGE` (256). Using a buffer of `MAX_MESSAGE + 1` bytes avoids this.
5. **Loose parsing.** `sscanf` matches prefixes, so `ADD 1 2 junk` is accepted as `ADD 1 2`. Also, `PUSH`, `ENQUEUE` etc. are checked in a fixed order, so command names must not share ambiguous prefixes.
6. **Blocking sends.** `mq_send` blocks if a queue already holds 10 messages. If the UI stops reading `/pambu_response`, or the Logger stops reading `/pambu_log`, the core will hang. `O_NONBLOCK` or `mq_timedsend` could prevent this.
7. **Signals.** Pressing Ctrl+C skips the cleanup section, leaving queues behind. (Startup `mq_unlink` mitigates this for the next run.) A `SIGINT` handler could close and unlink cleanly.
8. **Integer overflow.** `cpu_add` / `cpu_subtract` do not guard against `int` overflow.

---

## 9. Quick Summary

| Aspect | Detail |
|---|---|
| Role | CPU core: memory, stack, queue, add/subtract |
| IPC | POSIX message queues (request in, response out, log out) |
| Input | Text commands from the UI |
| Output | Text result to UI + log line to Logger |
| Dependencies | Logger must be running first; link with `-lrt` |
| Stop | Send the `exit` command |
