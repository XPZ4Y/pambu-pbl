#include "schedule.h"

/*
 * Dataset for both SJF modes:
 * P1: AT=0, BT=7
 * P2: AT=2, BT=4
 * P3: AT=4, BT=1
 * P4: AT=5, BT=4
 *
 * Non-Preemptive SJF Execution:
 * [0..7] P1 (CT=7, TAT=7, WT=0, RT=0)
 * At t=7, P2(4), P3(1), P4(4) are waiting -> P3 is shortest
 * [7..8] P3 (CT=8, TAT=4, WT=3, RT=3)
 * [8..12] P2 (CT=12, TAT=10, WT=6, RT=6)
 * [12..16] P4 (CT=16, TAT=11, WT=7, RT=7)
 * Avg WT = (0+6+3+7)/4 = 4.00 | Avg TAT = (7+10+4+11)/4 = 8.00
 */
static bool verify_sjf_non_preemptive(void) {
    Process p[4] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 7, .priority = 1},
        {.pid = 2, .arrival_time = 2, .burst_time = 4, .priority = 1},
        {.pid = 3, .arrival_time = 4, .burst_time = 1, .priority = 1},
        {.pid = 4, .arrival_time = 5, .burst_time = 4, .priority = 1}
    };
    ScheduleMetrics m = {0};

    if (schedule_sjf_non_preemptive(p, 4, &m) != 0) return false;

    const Process *p3 = find_proc(p, 4, 3);
    return (p3 != NULL && p3->waiting_time == 3 && p3->turnaround_time == 4) &&
           FLOAT_EQ(m.avg_waiting_time, 4.0f) &&
           FLOAT_EQ(m.avg_turnaround_time, 8.0f);
}

/*
 * Preemptive SJF (SRTF) Execution on the same dataset:
 * [0..2] P1 runs (rem=5, RT=0)
 * [2..4] P2 preempts P1 (rem=2, RT=0)
 * [4..5] P3 preempts P2 (rem=0, CT=5, TAT=1, WT=0, RT=0)
 * [5..7] P2 resumes & finishes (CT=7, TAT=5, WT=1, RT=0)
 * [7..11] P4 runs (CT=11, TAT=6, WT=2, RT=2)
 * [11..16] P1 resumes & finishes (CT=16, TAT=16, WT=9, RT=0)
 * Avg WT = (9+1+0+2)/4 = 3.00 | Avg TAT = (16+5+1+6)/4 = 7.00
 */
static bool verify_sjf_preemptive(void) {
    Process p[4] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 7, .priority = 1},
        {.pid = 2, .arrival_time = 2, .burst_time = 4, .priority = 1},
        {.pid = 3, .arrival_time = 4, .burst_time = 1, .priority = 1},
        {.pid = 4, .arrival_time = 5, .burst_time = 4, .priority = 1}
    };
    ScheduleMetrics m = {0};

    if (schedule_sjf_preemptive(p, 4, &m) != 0) return false;

    const Process *p1 = find_proc(p, 4, 1);
    const Process *p2 = find_proc(p, 4, 2);
    if (!p1 || !p2) return false;

    return (p1->waiting_time == 9 && p1->response_time == 0) &&
           (p2->waiting_time == 1 && p2->response_time == 0) &&
           FLOAT_EQ(m.avg_waiting_time, 3.0f) &&
           FLOAT_EQ(m.avg_turnaround_time, 7.0f) &&
           (m.gantt_len >= 5);
}

int main(void) {
    printf("Running suite: test_sjf\n");

    TEST_FUNC(schedule_sjf_non_preemptive, verify_sjf_non_preemptive());
    TEST_FUNC(schedule_sjf_preemptive, verify_sjf_preemptive());
    TEST_FUNC(schedule_sjf_non_preemptive, schedule_sjf_non_preemptive(NULL, 0, NULL) == -1);
    TEST_FUNC(schedule_sjf_preemptive, schedule_sjf_preemptive(NULL, 0, NULL) == -1);

    if (failures > 0) {
        printf("[RESULT] TEST FAILED (%d failure(s))\n\n", failures);
        return 1;
    }
    printf("[RESULT] ALL TESTS PASSED\n\n");
    return 0;
}
