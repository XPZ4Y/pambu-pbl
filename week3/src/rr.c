
#include "schedule.h"

static int enqueue_arrived(Process *p, int n, int time,
                           int *admitted, int *q, int cap,
                           int *rear, int *count)
{
    for (;;) {
        int best = -1;

        for (int i = 0; i < n; i++) {
            if (!admitted[i] && p[i].arrival_time <= time) {
                if (best == -1 ||
                    p[i].arrival_time < p[best].arrival_time ||
                    (p[i].arrival_time == p[best].arrival_time &&
                     p[i].pid < p[best].pid))
                    best = i;
            }
        }

        if (best == -1)
            break;

        if (*count >= cap)
            return -1;

        q[*rear] = best;
        *rear = (*rear + 1) % cap;
        (*count)++;
        admitted[best] = 1;
    }

    return 0;
}

int schedule_rr(Process *procs, int n, int time_quantum,
                ScheduleMetrics *metrics)
{
    if (procs == NULL || metrics == NULL ||
        n <= 0 || time_quantum <= 0)
        return -1;

    for (int i = 0; i < n; i++) {
        if (procs[i].arrival_time < 0 ||
            procs[i].burst_time <= 0)
            return -1;
    }

    int remaining[n], admitted[n], queue[n + 1];
    int cap = n + 1;
    int front = 0, rear = 0, count = 0;
    int time = 0, completed = 0, busy = 0;
    float total_wt = 0, total_tat = 0, total_rt = 0;

    metrics->gantt_len = 0;
    metrics->avg_waiting_time = 0;
    metrics->avg_turnaround_time = 0;
    metrics->avg_response_time = 0;
    metrics->cpu_utilization = 0;

    for (int i = 0; i < n; i++) {
        remaining[i] = procs[i].burst_time;
        admitted[i] = 0;

        procs[i].start_time = -1;
        procs[i].completion_time = 0;
        procs[i].turnaround_time = 0;
        procs[i].waiting_time = 0;
        procs[i].response_time = 0;
    }

    while (completed < n) {
        if (enqueue_arrived(procs, n, time, admitted,
                            queue, cap, &rear, &count) != 0)
            return -1;

        if (count == 0) {
            int next = -1;

            for (int i = 0; i < n; i++) {
                if (remaining[i] > 0 &&
                    (next == -1 ||
                     procs[i].arrival_time <
                         procs[next].arrival_time ||
                     (procs[i].arrival_time ==
                          procs[next].arrival_time &&
                      procs[i].pid < procs[next].pid)))
                    next = i;
            }

            if (next == -1)
                break;

            int idle_end = procs[next].arrival_time;

            if (idle_end > time) {
                if (metrics->gantt_len >= MAX_GANTT_SLICES)
                    return -1;

                metrics->gantt[metrics->gantt_len++] =
                    (GanttSlice){-1, time, idle_end};

                time = idle_end;
            }

            if (enqueue_arrived(procs, n, time, admitted,
                                queue, cap, &rear, &count) != 0)
                return -1;
        }

        if (count == 0)
            continue;

        int idx = queue[front];
        front = (front + 1) % cap;
        count--;

        int start = time;

        if (procs[idx].start_time == -1) {
            procs[idx].start_time = time;
            procs[idx].response_time =
                time - procs[idx].arrival_time;
        }

        int run = remaining[idx] < time_quantum
                      ? remaining[idx] : time_quantum;

        time += run;
        remaining[idx] -= run;
        busy += run;

        if (metrics->gantt_len >= MAX_GANTT_SLICES)
            return -1;

        metrics->gantt[metrics->gantt_len++] =
            (GanttSlice){procs[idx].pid, start, time};

        /* Add newly arrived processes before re-queueing
           the current process. */
        if (enqueue_arrived(procs, n, time, admitted,
                            queue, cap, &rear, &count) != 0)
            return -1;

        if (remaining[idx] > 0) {
            if (count >= cap)
                return -1;

            queue[rear] = idx;
            rear = (rear + 1) % cap;
            count++;
        } else {
            completed++;

            procs[idx].completion_time = time;
            procs[idx].turnaround_time =
                time - procs[idx].arrival_time;
            procs[idx].waiting_time =
                procs[idx].turnaround_time - procs[idx].burst_time;
        }
    }

    for (int i = 0; i < n; i++) {
        total_wt += procs[i].waiting_time;
        total_tat += procs[i].turnaround_time;
        total_rt += procs[i].response_time;
    }

    metrics->avg_waiting_time = total_wt / n;
    metrics->avg_turnaround_time = total_tat / n;
    metrics->avg_response_time = total_rt / n;

    if (time > 0)
        metrics->cpu_utilization =
            ((float)busy / time) * 100.0f;

    return 0;
}
