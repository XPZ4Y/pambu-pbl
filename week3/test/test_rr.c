#include "schedule.h"

/*
 * Round Robin Test Case (Time Quantum = 2):
 * P1: AT=0, BT=5
 * P2: AT=1, BT=3
 * P3: AT=2, BT=1
 *
 * Ready Queue & Execution Timeline:
 * [0..2] P1 runs (rem=3, RT=0). P2(t=1) and P3(t=2) arrive before/at t=2 -> Queue: [P2, P3, P1]
 * [2..4] P2 runs (rem=1, RT=1). Queue: [P3, P1, P2]
 * [4..5] P3 runs & finishes (rem=0, CT=5, TAT=3, WT=2, RT=2). Queue: [P1, P2]
 * [5..7] P1 runs (rem=1). Queue: [P2, P1]
 * [7..8] P2 runs & finishes (rem=0, CT=8, TAT=7, WT=4, RT=1). Queue: [P1]
 * [8..9] P1 runs & finishes (rem=0, CT=9, TAT=9, WT=4, RT=0).
 *
 * Avg WT = (4+4+2)/3 = 3.33 | Avg TAT = (9+7+3)/3 = 6.33 | Gantt Slices = 6
 */
static bool verify_rr_quantum_2(void) {
    Process p[3] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 5, .priority = 1},
        {.pid = 2, .arrival_time = 1, .burst_time = 3, .priority = 1},
        {.pid = 3, .arrival_time = 2, .burst_time = 1, .priority = 1}
    };
    ScheduleMetrics m = {0};

    if (schedule_rr(p, 3, 2, &m) != 0) return false;

    const Process *p1 = find_proc(p, 3, 1);
    const Process *p2 = find_proc(p, 3, 2);
    const Process *p3 = find_proc(p, 3, 3);
    if (!p1 || !p2 || !p3) return false;

    return (p1->completion_time == 9 && p1->waiting_time == 4 && p1->response_time == 0) &&
           (p2->completion_time == 8 && p2->waiting_time == 4 && p2->response_time == 1) &&
           (p3->completion_time == 5 && p3->waiting_time == 2 && p3->response_time == 2) &&
           FLOAT_EQ(m.avg_waiting_time, 3.33f) &&
           FLOAT_EQ(m.avg_turnaround_time, 6.33f) &&
           (m.gantt_len == 6);
}

static bool verify_rr_invalid_quantum(void) {
    Process p[1] = {{.pid = 1, .arrival_time = 0, .burst_time = 4, .priority = 1}};
    ScheduleMetrics m = {0};
    return (schedule_rr(p, 1, 0, &m) == -1) && (schedule_rr(p, 1, -2, &m) == -1);
}

int main(void) {
    printf("Running suite: test_rr\n");

    TEST_FUNC(schedule_rr, verify_rr_quantum_2());
    TEST_FUNC(schedule_rr, verify_rr_invalid_quantum());
    TEST_FUNC(schedule_rr, schedule_rr(NULL, 0, 2, NULL) == -1);

    if (failures > 0) {
        printf("[RESULT] TEST FAILED (%d failure(s))\n\n", failures);
        return 1;
    }
    printf("[RESULT] ALL TESTS PASSED\n\n");
    return 0;
}
