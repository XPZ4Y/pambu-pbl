#include "schedule.h"
#include <stdlib.h>
#include <string.h>

static void clear_metrics(Process *p, int n) {
    for (int i = 0; i < n; i++) {
        p[i].start_time = 0;
        p[i].completion_time = 0;
        p[i].turnaround_time = 0;
        p[i].waiting_time = 0;
        p[i].response_time = 0;
    }
}

static void push_slice(ScheduleMetrics *m, int pid, int s, int e) {
    if (e <= s) return;
    if (m->gantt_len > 0) {
        GanttSlice *last = &m->gantt[m->gantt_len - 1];
        if (last->pid == pid && last->end_time == s) {
            last->end_time = e;
            return;
        }
    }
    if (m->gantt_len < MAX_GANTT_SLICES) {
        m->gantt[m->gantt_len].pid = pid;
        m->gantt[m->gantt_len].start_time = s;
        m->gantt[m->gantt_len].end_time = e;
        m->gantt_len++;
    }
}

static void wrap_up(Process *p, int n, ScheduleMetrics *m, int busy, int span) {
    float sw = 0, st = 0, sr = 0;
    for (int i = 0; i < n; i++) {
        sw += p[i].waiting_time;
        st += p[i].turnaround_time;
        sr += p[i].response_time;
    }
    m->avg_waiting_time = sw / n;
    m->avg_turnaround_time = st / n;
    m->avg_response_time = sr / n;
    m->cpu_utilization = span > 0 ? ((float)busy / span) * 100.0f : 0.0f;
}

int schedule_sjf_non_preemptive(Process *procs, int n, ScheduleMetrics *metrics) {
    if (!procs || !metrics || n <= 0) return -1;

    memset(metrics, 0, sizeof(*metrics));
    clear_metrics(procs, n);

    int *done = calloc(n, sizeof(int));
    if (!done) return -1;

    int t = 0, finished = 0, first = -1;

    while (finished < n) {
        int pick = -1;

        for (int i = 0; i < n; i++) {
            if (done[i] || procs[i].arrival_time > t) continue;
            if (pick == -1) { pick = i; continue; }
            if (procs[i].burst_time < procs[pick].burst_time) { pick = i; continue; }
            if (procs[i].burst_time == procs[pick].burst_time &&
                procs[i].arrival_time < procs[pick].arrival_time) { pick = i; continue; }
            if (procs[i].burst_time == procs[pick].burst_time &&
                procs[i].arrival_time == procs[pick].arrival_time &&
                procs[i].pid < procs[pick].pid) { pick = i; }
        }

        if (pick == -1) {
            int nxt = -1;
            for (int i = 0; i < n; i++) {
                if (done[i]) continue;
                if (nxt == -1 || procs[i].arrival_time < procs[nxt].arrival_time)
                    nxt = i;
            }
            push_slice(metrics, -1, t, procs[nxt].arrival_time);
            t = procs[nxt].arrival_time;
            continue;
        }

        procs[pick].start_time = t;
        procs[pick].completion_time = t + procs[pick].burst_time;
        procs[pick].turnaround_time = procs[pick].completion_time - procs[pick].arrival_time;
        procs[pick].waiting_time = procs[pick].turnaround_time - procs[pick].burst_time;
        procs[pick].response_time = procs[pick].start_time - procs[pick].arrival_time;

        push_slice(metrics, procs[pick].pid, t, procs[pick].completion_time);

        if (first == -1) first = t;

        t = procs[pick].completion_time;
        done[pick] = 1;
        finished++;
    }

    int busy = 0;
    for (int i = 0; i < n; i++) busy += procs[i].burst_time;
    int span = t - first;
    if (span <= 0) span = t;

    wrap_up(procs, n, metrics, busy, span);
    free(done);
    return 0;
}

int schedule_sjf_preemptive(Process *procs, int n, ScheduleMetrics *metrics) {
    if (!procs || !metrics || n <= 0) return -1;

    memset(metrics, 0, sizeof(*metrics));
    clear_metrics(procs, n);

    int *done = calloc(n, sizeof(int));
    int *left = calloc(n, sizeof(int));
    int *seen = calloc(n, sizeof(int));
    if (!done || !left || !seen) {
        free(done); free(left); free(seen);
        return -1;
    }

    for (int i = 0; i < n; i++) left[i] = procs[i].burst_time;

    int t = procs[0].arrival_time;
    for (int i = 1; i < n; i++)
        if (procs[i].arrival_time < t) t = procs[i].arrival_time;

    int finished = 0, first = -1;

    while (finished < n) {
        int pick = -1;

        for (int i = 0; i < n; i++) {
            if (done[i] || procs[i].arrival_time > t) continue;
            if (pick == -1) { pick = i; continue; }
            if (left[i] < left[pick]) { pick = i; continue; }
            if (left[i] == left[pick] &&
                procs[i].arrival_time < procs[pick].arrival_time) { pick = i; continue; }
            if (left[i] == left[pick] &&
                procs[i].arrival_time == procs[pick].arrival_time &&
                procs[i].pid < procs[pick].pid) { pick = i; }
        }

        if (pick == -1) {
            int nxt = -1;
            for (int i = 0; i < n; i++) {
                if (done[i]) continue;
                if (nxt == -1 || procs[i].arrival_time < procs[nxt].arrival_time)
                    nxt = i;
            }
            push_slice(metrics, -1, t, procs[nxt].arrival_time);
            t = procs[nxt].arrival_time;
            continue;
        }

        if (!seen[pick]) {
            procs[pick].start_time = t;
            procs[pick].response_time = t - procs[pick].arrival_time;
            seen[pick] = 1;
        }

        if (first == -1) first = t;

        int stop = t + left[pick];
        for (int i = 0; i < n; i++) {
            if (done[i] || i == pick) continue;
            if (procs[i].arrival_time > t &&
                procs[i].arrival_time < stop &&
                procs[i].burst_time < left[pick]) {
                stop = procs[i].arrival_time;
            }
        }

        push_slice(metrics, procs[pick].pid, t, stop);
        left[pick] -= (stop - t);
        t = stop;

        if (left[pick] == 0) {
            procs[pick].completion_time = t;
            procs[pick].turnaround_time = procs[pick].completion_time - procs[pick].arrival_time;
            procs[pick].waiting_time = procs[pick].turnaround_time - procs[pick].burst_time;
            done[pick] = 1;
            finished++;
        }
    }

    int busy = 0;
    for (int i = 0; i < n; i++) busy += procs[i].burst_time;
    int span = t - first;
    if (span <= 0) span = t;

    wrap_up(procs, n, metrics, busy, span);

    free(done);
    free(left);
    free(seen);
    return 0;
}

int tested_main(void) {
    Process p[4] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 7, .priority = 1},
        {.pid = 2, .arrival_time = 2, .burst_time = 4, .priority = 1},
        {.pid = 3, .arrival_time = 4, .burst_time = 1, .priority = 1},
        {.pid = 4, .arrival_time = 5, .burst_time = 4, .priority = 1}
    };
    ScheduleMetrics m = {0};

    schedule_sjf_non_preemptive(p, 4, &m);

    printf("PID  Arr  Burst  Start  CT  TAT  WT  RT\n");
    for (int i = 0; i < 4; i++) {
        printf("%-4d %-4d %-6d %-6d %-3d %-4d %-3d %d\n",
               p[i].pid, p[i].arrival_time, p[i].burst_time,
               p[i].start_time, p[i].completion_time,
               p[i].turnaround_time, p[i].waiting_time, p[i].response_time);
    }
    printf("Avg WT=%.2f Avg TAT=%.2f Avg RT=%.2f CPU=%.2f%%\n",
           m.avg_waiting_time, m.avg_turnaround_time,
           m.avg_response_time, m.cpu_utilization);

    return 0;
}
