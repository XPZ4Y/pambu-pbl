
// src/fcfs.c
#include "schedule.h"
#include <string.h>

int schedule_fcfs(Process *procs, int n, ScheduleMetrics *metrics) {
    // Validate inputs
    if (procs == NULL || metrics == NULL || n <= 0) {
        return -1;
    }

    // Initialize metrics
    memset(metrics, 0, sizeof(ScheduleMetrics));

    // Sort process indices by arrival time (FCFS tie-breaker: lower pid first).
    // We use an index array instead of copying Process structs, so we don't
    // need dynamic allocation and we can write directly into the original array.
    int order[n];
    for (int i = 0; i < n; i++) {
        order[i] = i;
    }

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            Process *a = &procs[order[i]];
            Process *b = &procs[order[j]];
            if (b->arrival_time < a->arrival_time ||
                (b->arrival_time == a->arrival_time && b->pid < a->pid)) {
                int tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }

    int current_time = 0;
    int total_busy = 0;
    int gantt_idx = 0;
    double total_wt = 0, total_tat = 0, total_rt = 0;

    for (int i = 0; i < n; i++) {
        Process *p = &procs[order[i]];

        // If CPU is idle before this process arrives, record an idle slice
        if (current_time < p->arrival_time) {
            if (gantt_idx < MAX_GANTT_SLICES) {
                metrics->gantt[gantt_idx].pid = -1;
                metrics->gantt[gantt_idx].start_time = current_time;
                metrics->gantt[gantt_idx].end_time = p->arrival_time;
                gantt_idx++;
            }
            current_time = p->arrival_time;
        }

        // Record Gantt slice for this process
        if (gantt_idx < MAX_GANTT_SLICES) {
            metrics->gantt[gantt_idx].pid = p->pid;
            metrics->gantt[gantt_idx].start_time = current_time;
            metrics->gantt[gantt_idx].end_time = current_time + p->burst_time;
            gantt_idx++;
        }

        // Compute per-process metrics
        p->start_time = current_time;
        p->completion_time = current_time + p->burst_time;
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->burst_time;
        p->response_time = p->start_time - p->arrival_time;

        total_wt += p->waiting_time;
        total_tat += p->turnaround_time;
        total_rt += p->response_time;

        current_time = p->completion_time;
        total_busy += p->burst_time;
    }

    // Compute averages
    metrics->avg_waiting_time = (float)(total_wt / n);
    metrics->avg_turnaround_time = (float)(total_tat / n);
    metrics->avg_response_time = (float)(total_rt / n);

    // CPU utilization = (total busy time / total elapsed time) * 100
    if (current_time > 0) {
        metrics->cpu_utilization = ((float)total_busy / (float)current_time) * 100.0f;
    } else {
        metrics->cpu_utilization = 0.0f;
    }

    metrics->gantt_len = gantt_idx;

    return 0;
}
