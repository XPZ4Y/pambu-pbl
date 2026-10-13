
#include "schedule.h"

static int better_process(const Process *a, const Process *b)
{
    if (a->priority != b->priority)
        return a->priority < b->priority;

    if (a->arrival_time != b->arrival_time)
        return a->arrival_time < b->arrival_time;

    return a->pid < b->pid;
}

static void add_gantt(ScheduleMetrics *m, int pid, int start, int end)
{
    if (start >= end)
        return;

    if (m->gantt_len > 0) {
        GanttSlice *last = &m->gantt[m->gantt_len - 1];

        if (last->pid == pid && last->end_time == start) {
            last->end_time = end;
            return;
        }
    }

    if (m->gantt_len < MAX_GANTT_SLICES) {
        GanttSlice *g = &m->gantt[m->gantt_len++];
        g->pid = pid;
        g->start_time = start;
        g->end_time = end;
    }
}

static int valid_input(Process *p, int n, ScheduleMetrics *m)
{
    if (p == NULL || m == NULL || n <= 0)
        return 0;

    for (int i = 0; i < n; i++) {
        if (p[i].arrival_time < 0 || p[i].burst_time <= 0)
            return 0;
    }

    return 1;
}

static void reset_metrics(ScheduleMetrics *m)
{
    m->avg_waiting_time = 0.0f;
    m->avg_turnaround_time = 0.0f;
    m->avg_response_time = 0.0f;
    m->cpu_utilization = 0.0f;
    m->gantt_len = 0;
}

static void calculate_metrics(Process *p, int n,
                              ScheduleMetrics *m,
                              int total_busy, int finish_time)
{
    float total_wait = 0.0f;
    float total_turnaround = 0.0f;
    float total_response = 0.0f;

    for (int i = 0; i < n; i++) {
        p[i].turnaround_time =
            p[i].completion_time - p[i].arrival_time;

        p[i].waiting_time =
            p[i].turnaround_time - p[i].burst_time;

        p[i].response_time =
            p[i].start_time - p[i].arrival_time;

        total_wait += p[i].waiting_time;
        total_turnaround += p[i].turnaround_time;
        total_response += p[i].response_time;
    }

    m->avg_waiting_time = total_wait / n;
    m->avg_turnaround_time = total_turnaround / n;
    m->avg_response_time = total_response / n;

    if (finish_time > 0)
        m->cpu_utilization =
            (float)total_busy * 100.0f / finish_time;
}

/* Non-preemptive Priority Scheduling */
int schedule_priority_non_preemptive(
    Process *procs, int n, ScheduleMetrics *metrics)
{
    if (!valid_input(procs, n, metrics))
        return -1;

    reset_metrics(metrics);

    bool done[n];
    int completed = 0;
    int time = 0;
    int total_busy = 0;

    for (int i = 0; i < n; i++) {
        done[i] = false;
        procs[i].start_time = -1;
        procs[i].completion_time = 0;
        procs[i].turnaround_time = 0;
        procs[i].waiting_time = 0;
        procs[i].response_time = 0;
    }

    while (completed < n) {
        int best = -1;

        for (int i = 0; i < n; i++) {
            if (!done[i] && procs[i].arrival_time <= time) {
                if (best == -1 ||
                    better_process(&procs[i], &procs[best])) {
                    best = i;
                }
            }
        }

        if (best == -1) {
            int next = -1;

            for (int i = 0; i < n; i++) {
                if (!done[i] &&
                    (next == -1 ||
                     procs[i].arrival_time < procs[next].arrival_time)) {
                    next = i;
                }
            }

            add_gantt(metrics, -1, time, procs[next].arrival_time);
            time = procs[next].arrival_time;
            continue;
        }

        procs[best].start_time = time;
        add_gantt(metrics, procs[best].pid,
                  time, time + procs[best].burst_time);

        time += procs[best].burst_time;
        total_busy += procs[best].burst_time;
        procs[best].completion_time = time;
        done[best] = true;
        completed++;
    }

    calculate_metrics(procs, n, metrics, total_busy, time);
    return 0;
}

/* Preemptive Priority Scheduling */
int schedule_priority_preemptive(
    Process *procs, int n, ScheduleMetrics *metrics)
{
    if (!valid_input(procs, n, metrics))
        return -1;

    reset_metrics(metrics);

    int remaining[n];
    int completed = 0;
    int time = 0;
    int total_busy = 0;

    for (int i = 0; i < n; i++) {
        remaining[i] = procs[i].burst_time;
        procs[i].start_time = -1;
        procs[i].completion_time = 0;
        procs[i].turnaround_time = 0;
        procs[i].waiting_time = 0;
        procs[i].response_time = 0;
    }

    while (completed < n) {
        int best = -1;

        for (int i = 0; i < n; i++) {
            if (remaining[i] > 0 &&
                procs[i].arrival_time <= time) {
                if (best == -1 ||
                    better_process(&procs[i], &procs[best])) {
                    best = i;
                }
            }
        }

        if (best == -1) {
            int next = -1;

            for (int i = 0; i < n; i++) {
                if (remaining[i] > 0 &&
                    (next == -1 ||
                     procs[i].arrival_time < procs[next].arrival_time)) {
                    next = i;
                }
            }

            add_gantt(metrics, -1, time, time + 1);
            time++;
            continue;
        }

        if (procs[best].start_time == -1)
            procs[best].start_time = time;

        add_gantt(metrics, procs[best].pid, time, time + 1);

        remaining[best]--;
        time++;
        total_busy++;

        if (remaining[best] == 0) {
            procs[best].completion_time = time;
            completed++;
        }
    }

    calculate_metrics(procs, n, metrics, total_busy, time);
    return 0;
}
