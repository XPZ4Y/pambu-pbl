#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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
    int        page;                  /* requested page number          */
    int        frames[MAX_FRAMES];    /* frame contents after request   */
    int        frame_count;           /* number of frames in use        */
    PageStatus status;                /* hit or fault                    */
    int        victim_frame;          /* index replaced (-1 if none)    */
} SimStep;

/* ---------- Per-algorithm statistics + trace ---------- */
typedef struct {
    const char *name;                 /* "FIFO", "LRU", "Optimal"       */
    AlgorithmChoice algo;
    int  frames;                      /* frame count used               */
    int  total_refs;
    int  hits;
    int  faults;
    SimStep steps[MAX_REFERENCES];    /* step-by-step trace             */
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

/* ==================== orchestrator.c ==================== */
int  read_input(SimInput *in);
int  validate_input(const SimInput *in);
void run_simulation(const SimInput *in, SimResult results[], int *result_count);
void print_comparison(const SimResult results[], int result_count);
int  find_best_algorithm(const SimResult results[], int result_count);

/* ==================== fifo.c ==================== */
void simulate_fifo(const AlgoInput *in, SimResult *out);

/* ==================== lru.c ==================== */
void simulate_lru(const AlgoInput *in, SimResult *out);

/* ==================== optimal.c ==================== */
void simulate_optimal(const AlgoInput *in, SimResult *out);

/* ==================== display.c ==================== */
void print_header(const char *title);
void print_sim_table(const SimResult *r);
void print_stats(const SimResult *r);
void print_comparison_table(const SimResult results[], int count);

//Special drug
void result_init(SimResult *out, const char *name,
                 AlgorithmChoice algo, const AlgoInput *in);

#endif /* ORCHESTRATOR_H */
