
#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define WEAK __attribute__((weak))
#define MAX_GANTT_SLICES 256
#define EPSILON 0.05f

/* --- 1. CORE DATA STRUCTURES --- */

typedef struct {
    int pid;             // Input: Process ID
    int arrival_time;    // Input: Arrival Time (AT)
    int burst_time;      // Input: Burst Time (BT)
    int priority;        // Input: Priority (Lower numerical value = Higher priority)

    // Computed per-process metrics
    int start_time;      // First time CPU is allocated to this process
    int completion_time; // Time process finishes execution (CT)
    int turnaround_time; // TAT = CT - AT
    int waiting_time;    // WT  = TAT - BT
    int response_time;   // RT  = start_time - AT
} Process;

typedef struct {
    int pid;             // Process ID (-1 indicates CPU IDLE time)
    int start_time;      // Slice start timestamp
    int end_time;        // Slice end timestamp
} GanttSlice;

typedef struct {
    float avg_waiting_time;    // Average WT across all n processes
    float avg_turnaround_time; // Average TAT across all n processes
    float avg_response_time;   // Average RT across all n processes
    float cpu_utilization;     // (Total Busy Time / Total Elapsed Time) * 100.0f
    GanttSlice gantt[MAX_GANTT_SLICES];
    int gantt_len;             // Number of recorded Gantt chart slices
} ScheduleMetrics;

/* --- 2. REQUIRED ALGORITHM SIGNATURES (WEAK SYMBOLS FOR TDD) --- */
/*
 * CONVENTIONS FOR INTERNS:
 * 1. Every src/<algo>.c file MUST #include "schedule.h"
 * 2. Return 0 on valid execution, or -1 on invalid input (e.g., n <= 0, NULL pointers, time_quantum <= 0).
 * 3. Priority convention: Lower integer = Higher priority (e.g., priority 1 runs before priority 3).
 * 4. Tie-breaking convention: Break ties by earlier arrival_time, then by lower pid.
 */

// src/fcfs.c
WEAK int schedule_fcfs(Process *procs, int n, ScheduleMetrics *metrics);

// src/sjf.c
WEAK int schedule_sjf_non_preemptive(Process *procs, int n, ScheduleMetrics *metrics);
WEAK int schedule_sjf_preemptive(Process *procs, int n, ScheduleMetrics *metrics);

// src/priority.c
WEAK int schedule_priority_non_preemptive(Process *procs, int n, ScheduleMetrics *metrics);
WEAK int schedule_priority_preemptive(Process *procs, int n, ScheduleMetrics *metrics);

// src/rr.c
WEAK int schedule_rr(Process *procs, int n, int time_quantum, ScheduleMetrics *metrics);

// Optional standalone main inside src/<algo>.c (renamed via -Dmain=tested_main)
WEAK int tested_main(void);

/* --- 3. SHARED TEST HARNESS UTILITIES --- */

static int failures = 0;

#define FLOAT_EQ(a, b) (fabsf((a) - (b)) < EPSILON)

#define TEST_FUNC(func_ptr, test_expr) do { \
    if ((func_ptr) == NULL) { \
        printf("  [FAIL] '%s' is not implemented.\n", #func_ptr); \
        failures++; \
    } else if (!(test_expr)) { \
        printf("  [FAIL] Assertion failed: %s\n", #test_expr); \
        failures++; \
    } else { \
        printf("  [PASS] %s\n", #test_expr); \
    } \
} while (0)

static inline const Process* find_proc(const Process *procs, int n, int pid) {
    for (int i = 0; i < n; i++) {
        if (procs[i].pid == pid) return &procs[i];
    }
    return NULL;
}

#endif // SCHEDULE_H
