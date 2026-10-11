#include "schedule.h"

/*
 * Dataset:
 * P1: AT=0, BT=4, Priority=2
 * P2: AT=1, BT=3, Priority=1 (Highest priority)
 * P3: AT=2, BT=1, Priority=3 (Lowest priority)
 *
 * Non-Preemptive Priority Execution:
 * [0..4] P1 runs to completion (CT=4, TAT=4, WT=0, RT=0)
 * [4..7] P2 runs next (prio 1 < prio 3) (CT=7, TAT=6, WT=3, RT=3)
 * [7..8] P3 runs last (CT=8, TAT=6, WT=5, RT=5)
 * Avg WT = (0+3+5)/3 = 2.67 | Avg TAT = (4+6+6)/3 = 5.33
 */
static bool verify_priority_non_preemptive(void) {
    Process p[3] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 4, .priority = 2},
        {.pid = 2, .arrival_time = 1, .burst_time = 3, .priority = 1},
        {.pid = 3, .arrival_time = 2, .burst_time = 1, .priority = 3}
    };
    ScheduleMetrics m = {0};

    if (schedule_priority_non_preemptive(p, 3, &m) != 0) return false;

    const Process *p2 = find_proc(p, 3, 2);
    return (p2 != NULL && p2->waiting_time == 3 && p2->turnaround_time == 6) &&
           FLOAT_EQ(m.avg_waiting_time, 2.67f) &&
           FLOAT_EQ(m.avg_turnaround_time, 5.33f);
}

/*
 * Preemptive Priority Execution on the same dataset:
 * [0..1] P1 starts (rem=3, RT=0)
 * [1..4] P2 arrives with Priority 1 and preempts P1 (CT=4, TAT=3, WT=0, RT=0)
 * [4..7] P1 resumes & finishes (CT=7, TAT=7, WT=3, RT=0)
 * [7..8] P3 runs (CT=8, TAT=6, WT=5, RT=5)
 * Avg WT = (3+0+5)/3 = 2.67 | Avg TAT = (7+3+6)/3 = 5.33
 */
static bool verify_priority_preemptive(void) {
    Process p[3] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 4, .priority = 2},
        {.pid = 2, .arrival_time = 1, .burst_time = 3, .priority = 1},
        {.pid = 3, .arrival_time = 2, .burst_time = 1, .priority = 3}
    };
    ScheduleMetrics m = {0};

    if (schedule_priority_preemptive(p, 3, &m) != 0) return false;

    const Process *p1 = find_proc(p, 3, 1);
    const Process *p2 = find_proc(p, 3, 2);
    if (!p1 || !p2) return false;

    return (p1->waiting_time == 3 && p1->turnaround_time == 7 && p1->response_time == 0) &&
           (p2->waiting_time == 0 && p2->turnaround_time == 3 && p2->response_time == 0) &&
           FLOAT_EQ(m.avg_waiting_time, 2.67f) &&
           FLOAT_EQ(m.avg_turnaround_time, 5.33f) &&
           (m.gantt_len >= 4);
}

int main(void) {
    printf("Running suite: test_priority\n");

    TEST_FUNC(schedule_priority_non_preemptive, verify_priority_non_preemptive());
    TEST_FUNC(schedule_priority_preemptive, verify_priority_preemptive());
    TEST_FUNC(schedule_priority_non_preemptive, schedule_priority_non_preemptive(NULL, 0, NULL) == -1);
    TEST_FUNC(schedule_priority_preemptive, schedule_priority_preemptive(NULL, 0, NULL) == -1);

    if (failures > 0) {
        printf("[RESULT] TEST FAILED (%d failure(s))\n\n", failures);
        return 1;
    }
    printf("[RESULT] ALL TESTS PASSED\n\n");
    return 0;
}
