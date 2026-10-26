#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>

/* ---------- Configuration constants ---------- */
#define MAX_FRAMES       64
#define MAX_REFERENCES   1024
#define EMPTY_FRAME      (-1)

/* ---------- Algorithm identifiers ---------- */
typedef enum {
    ALGO_FIFO = 1,
    ALGO_LRU  = 2,
    ALGO_OPT  = 3,
    ALGO_ALL  = 4
} AlgorithmChoice;

/* ---------- Status of a single page reference ---------- */
typedef enum {
    STATUS_FAULT = 0,
    STATUS_HIT   = 1
} PageStatus;

/* ---------- One row of the simulation table ---------- */
typedef struct {
    int        page;
    int        frames[MAX_FRAMES];
    int        frame_count;
    PageStatus status;
    int        victim_frame;
} SimStep;

/* ---------- Per-algorithm statistics + trace ---------- */
typedef struct {
    const char *name;
    AlgorithmChoice algo;
    int  frames;
    int  total_refs;
    int  hits;
    int  faults;
    SimStep steps[MAX_REFERENCES];
    int  step_count;
} SimResult;

/* ---------- User input bundle ---------- */
typedef struct {
    int            refs[MAX_REFERENCES];
    int            ref_count;
    int            frame_count;
    AlgorithmChoice choice;
} SimInput;

/* ---------- Raw per-reference input for a single algorithm ---------- */
typedef struct {
    int refs[MAX_REFERENCES];
    int ref_count;
    int frame_count;
} AlgoInput;

/* ---------- Function pointer: one algorithm = one function ---------- */
typedef void (*PageReplacementFn)(const AlgoInput *in, SimResult *out);

/* ==================== IPC backend protocol ====================
 *
 * These types are shared by backend.c (server) and any C-side test
 * harness. The Python frontend speaks the wire protocol directly
 * (newline-delimited JSON over a Unix domain socket); these types
 * are the in-process mirror of that protocol.
 *
 * Layout:
 *   backend.c   ── parses JSON → fills IpcRequest
 *               ── calls run_simulation()
 *               ── serializes IpcResponse → JSON → socket
 *
 * Wire protocol version is bumped on any incompatible change.
 * ============================================================ */

#define IPC_PROTOCOL_VERSION 1
#define IPC_MAX_ERROR_LEN    256
#define IPC_MAX_LINE_LEN     65536

/* Status codes returned by backend and mirrored in the JSON response.
 * Values are part of the wire contract — do not reorder. */
typedef enum {
    IPC_OK           =  0,   /* request handled successfully          */
    IPC_ERR_PARSE    = -1,   /* malformed JSON or missing field        */
    IPC_ERR_INPUT    = -2,   /* validate_input() rejected the request  */
    IPC_ERR_CORE     = -3,   /* simulation core not linked (stubs)     */
    IPC_ERR_INTERNAL = -4,   /* backend bug / allocation failure       */
    IPC_ERR_QUIT     = -5    /* client requested shutdown              */
} IpcStatus;

/* Which operation the client is asking for. */
typedef enum {
    IPC_CMD_RUN  = 1,        /* run one simulation (single algo or ALL)*/
    IPC_CMD_PING = 2,        /* health check — returns protocol version*/
    IPC_CMD_QUIT = 3         /* request server shutdown                */
} IpcCommand;

/* Parsed request from the client. Only the fields relevant to
 * `cmd` are populated; others are zero. */
typedef struct {
    IpcCommand cmd;
    SimInput   input;        /* valid when cmd == IPC_CMD_RUN          */
} IpcRequest;

/* Response to a single request. `results` is only populated when
 * `status == IPC_OK` and `cmd == IPC_CMD_RUN`. */
typedef struct {
    IpcStatus  status;
    int        protocol_version;
    int        result_count;             /* 0, 1, or 3 (ALGO_ALL)     */
    int        best_index;               /* -1 if not applicable      */
    SimResult  results[3];
    char       error[IPC_MAX_ERROR_LEN]; /* empty when status == OK   */
} IpcResponse;

/* ==================== Backend lifecycle (backend.c) ==================== */

/* One-time server setup: bind + listen on `socket_path`.
 * Removes a stale socket file if present.
 * Returns IPC_OK or IPC_ERR_INTERNAL. */
IpcStatus ipc_server_init(const char *socket_path);

/* Blocking accept-one-client + request loop until client sends
 * IPC_CMD_QUIT, disconnects, or an unrecoverable error occurs.
 * Returns IPC_OK on clean shutdown, IPC_ERR_INTERNAL otherwise. */
IpcStatus ipc_server_run(void);

/* Tear down the listening socket and unlink `socket_path`.
 * Safe to call whether or not ipc_server_run() was invoked. */
void ipc_server_shutdown(void);

/* These three are exposed for unit tests and for any future
 * C-side front-end (e.g. a REPL). They are the pure functions
 * backend.c uses internally — no socket I/O. */

/* Parse one newline-terminated JSON line into `req`.
 * Returns IPC_OK, IPC_ERR_PARSE, or IPC_ERR_QUIT. */
IpcStatus ipc_parse_request(const char *line, size_t len, IpcRequest *req);

/* Fill `resp` by running the simulation for `req`.
 * Calls run_simulation() / find_best_algorithm() internally. */
void ipc_dispatch(const IpcRequest *req, IpcResponse *resp);

/* Serialize `resp` as one JSON line (no trailing newline).
 * Returns bytes written, or 0 on buffer exhaustion. */
size_t ipc_serialize_response(const IpcResponse *resp,
                              char *buf, size_t cap);

/* ==================== Core (orchestrator.c) ==================== */
int  read_input(SimInput *in);
int  validate_input(const SimInput *in);
void run_simulation(const SimInput *in, SimResult results[], int *result_count);
void print_comparison(const SimResult results[], int result_count);
int  find_best_algorithm(const SimResult results[], int result_count);

/* ==================== fifo.c / lru.c / optimal.c ==================== */
void simulate_fifo(const AlgoInput *in, SimResult *out);
void simulate_lru (const AlgoInput *in, SimResult *out);
void simulate_optimal(const AlgoInput *in, SimResult *out);

/* ==================== display.c ==================== */
void print_header(const char *title);
void print_sim_table(const SimResult *r);
void print_stats(const SimResult *r);
void print_comparison_table(const SimResult results[], int count);

void result_init(SimResult *out, const char *name,
                 AlgorithmChoice algo, const AlgoInput *in);

#endif /* ORCHESTRATOR_H */
