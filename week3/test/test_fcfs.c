
#include "schedule.h"

/*
 * Test Case 1: Standard FCFS
 * P1: AT=0, BT=4 -> Start=0, CT=4,  TAT=4,  WT=0, RT=0
 * P2: AT=1, BT=3 -> Start=4, CT=7,  TAT=6,  WT=3, RT=3
 * P3: AT=2, BT=1 -> Start=7, CT=8,  TAT=6,  WT=5, RT=5
 * Avg WT = (0+3+5)/3 = 2.67 | Avg TAT = (4+6+6)/3 = 5.33 | CPU Util = 100.0%
 */
static bool verify_fcfs_basic(void) {
    Process p[3] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 4, .priority = 1},
        {.pid = 2, .arrival_time = 1, .burst_time = 3, .priority = 1},
        {.pid = 3, .arrival_time = 2, .burst_time = 1, .priority = 1}
    };
    ScheduleMetrics m = {0};

    if (schedule_fcfs(p, 3, &m) != 0) return false;

    const Process *p1 = find_proc(p, 3, 1);
    const Process *p2 = find_proc(p, 3, 2);
    const Process *p3 = find_proc(p, 3, 3);
    if (!p1 || !p2 || !p3) return false;

    return (p1->waiting_time == 0 && p1->turnaround_time == 4 && p1->response_time == 0) &&
           (p2->waiting_time == 3 && p2->turnaround_time == 6 && p2->response_time == 3) &&
           (p3->waiting_time == 5 && p3->turnaround_time == 6 && p3->response_time == 5) &&
           FLOAT_EQ(m.avg_waiting_time, 2.67f) &&
           FLOAT_EQ(m.avg_turnaround_time, 5.33f) &&
           FLOAT_EQ(m.cpu_utilization, 100.0f) &&
           (m.gantt_len >= 3);
}

/*
 * Test Case 2: FCFS with CPU Idle Time
 * P1: AT=0, BT=2 -> Runs [0..2], CT=2, TAT=2, WT=0
 * IDLE: [2..5] (3 units idle)
 * P2: AT=5, BT=3 -> Runs [5..8], CT=8, TAT=3, WT=0
 * Total busy = 5, Total elapsed = 8 -> CPU Util = (5/8)*100 = 62.5%
 */
static bool verify_fcfs_idle_gap(void) {
    Process p[2] = {
        {.pid = 1, .arrival_time = 0, .burst_time = 2, .priority = 1},
        {.pid = 2, .arrival_time = 5, .burst_time = 3, .priority = 1}
    };
    ScheduleMetrics m = {0};

    if (schedule_fcfs(p, 2, &m) != 0) return false;

    const Process *p2 = find_proc(p, 2, 2);
    return (p2 != NULL && p2->waiting_time == 0 && p2->turnaround_time == 3) &&
           FLOAT_EQ(m.avg_waiting_time, 0.0f) &&
           FLOAT_EQ(m.avg_turnaround_time, 2.5f) &&
           FLOAT_EQ(m.cpu_utilization, 62.5f);
}

int main(void) {
    printf("Running suite: test_fcfs\n");

    TEST_FUNC(schedule_fcfs, verify_fcfs_basic());
    TEST_FUNC(schedule_fcfs, verify_fcfs_idle_gap());
    TEST_FUNC(schedule_fcfs, schedule_fcfs(NULL, 0, NULL) == -1);

    if (failures > 0) {
        printf("[RESULT] TEST FAILED (%d failure(s))\n\n", failures);
        return 1;
    }
    printf("[RESULT] ALL TESTS PASSED\n\n");
    return 0;
}
