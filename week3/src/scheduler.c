#include "schedule.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macro that prints and immediately flushes stdout so prompts never hang in bash/make
#define PROMPT(...) do { \
    printf(__VA_ARGS__); \
    fflush(stdout); \
} while (0)

// Safe line-based integer reader (prevents scanf buffer lockups)
static int read_int(const char *prompt_text, int *out_val) {
    char buf[128];
    PROMPT("%s", prompt_text);
    if (!fgets(buf, sizeof(buf), stdin)) {
        return 0; // EOF
    }
    return (sscanf(buf, "%d", out_val) == 1);
}

static void print_gantt_chart(const ScheduleMetrics *m) {
    if (m->gantt_len <= 0) {
        printf("\n[No Gantt chart data recorded]\n");
        return;
    }

    printf("\nGantt Chart:\n ");
    for (int i = 0; i < m->gantt_len; i++) printf("------- ");
    printf("\n|");
    for (int i = 0; i < m->gantt_len; i++) {
        if (m->gantt[i].pid == -1)
            printf(" IDLE  |");
        else
            printf("  P%-3d |", m->gantt[i].pid);
    }
    printf("\n ");
    for (int i = 0; i < m->gantt_len; i++) printf("------- ");
    printf("\n%-8d", m->gantt[0].start_time);
    for (int i = 0; i < m->gantt_len; i++) {
        printf("%-8d", m->gantt[i].end_time);
    }
    printf("\n");
    fflush(stdout);
}

static void print_results(const Process *procs, int n, const ScheduleMetrics *m) {
    print_gantt_chart(m);

    printf("\n%-6s %-6s %-6s %-6s %-6s %-6s %-6s %-6s\n",
           "PID", "AT", "BT", "PRIO", "CT", "TAT", "WT", "RT");
    printf("-------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("P%-5d %-6d %-6d %-6d %-6d %-6d %-6d %-6d\n",
               procs[i].pid,
               procs[i].arrival_time,
               procs[i].burst_time,
               procs[i].priority,
               procs[i].completion_time,
               procs[i].turnaround_time,
               procs[i].waiting_time,
               procs[i].response_time);
    }
    printf("-------------------------------------------------------\n");
    printf("Average Waiting Time    : %.2f\n", m->avg_waiting_time);
    printf("Average Turnaround Time : %.2f\n", m->avg_turnaround_time);
    printf("Average Response Time   : %.2f\n", m->avg_response_time);
    printf("CPU Utilization         : %.2f%%\n\n", m->cpu_utilization);
    fflush(stdout);
}

int main(void) {
    // Disable stdout buffering globally
    setvbuf(stdout, NULL, _IONBF, 0);

    int n = 0;
    char line[128];

    printf("=== CPU PROCESS SCHEDULING SIMULATOR ===\n");
    fflush(stdout);

    if (!read_int("Enter number of processes: ", &n) || n <= 0) {
        fprintf(stderr, "Error: Number of processes must be greater than 0.\n");
        return 1;
    }

    Process *orig = calloc((size_t)n, sizeof(Process));
    Process *work = calloc((size_t)n, sizeof(Process));
    if (!orig || !work) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        free(orig);
        free(work);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        orig[i].pid = i + 1;
        while (1) {
            PROMPT("Process P%d [Arrival Burst Priority]: ", orig[i].pid);
            if (!fgets(line, sizeof(line), stdin)) {
                free(orig);
                free(work);
                return 1;
            }
            if (sscanf(line, "%d %d %d",
                       &orig[i].arrival_time,
                       &orig[i].burst_time,
                       &orig[i].priority) == 3) {
                if (orig[i].arrival_time >= 0 && orig[i].burst_time > 0) {
                    break;
                }
                printf("  [!] Arrival must be >= 0 and Burst must be > 0. Try again.\n");
            } else {
                printf("  [!] Please enter 3 space-separated integers (e.g., 0 5 1).\n");
            }
        }
    }

    while (1) {
        int choice = -1;
        int quantum = 2;
        int status = -1;
        ScheduleMetrics metrics = {0};

        // Reset working copy to original input values before running an algorithm
        memcpy(work, orig, (size_t)n * sizeof(Process));

        printf("\nSelect Scheduling Algorithm:\n");
        printf("  1. FCFS (First-Come-First-Served)\n");
        printf("  2. SJF (Non-Preemptive)\n");
        printf("  3. SJF (Preemptive / SRTF)\n");
        printf("  4. Priority (Non-Preemptive)\n");
        printf("  5. Priority (Preemptive)\n");
        printf("  6. Round Robin (RR)\n");
        printf("  0. Exit\n");
        fflush(stdout);

        if (!read_int("Choice: ", &choice) || choice == 0) {
            printf("Exiting simulator.\n");
            break;
        }

        switch (choice) {
            case 1:
                if (schedule_fcfs == NULL) {
                    printf("[!] FCFS is not implemented yet.\n");
                    continue;
                }
                status = schedule_fcfs(work, n, &metrics);
                break;

            case 2:
                if (schedule_sjf_non_preemptive == NULL) {
                    printf("[!] SJF (Non-Preemptive) is not implemented yet.\n");
                    continue;
                }
                status = schedule_sjf_non_preemptive(work, n, &metrics);
                break;

            case 3:
                if (schedule_sjf_preemptive == NULL) {
                    printf("[!] SJF (Preemptive) is not implemented yet.\n");
                    continue;
                }
                status = schedule_sjf_preemptive(work, n, &metrics);
                break;

            case 4:
                if (schedule_priority_non_preemptive == NULL) {
                    printf("[!] Priority (Non-Preemptive) is not implemented yet.\n");
                    continue;
                }
                status = schedule_priority_non_preemptive(work, n, &metrics);
                break;

            case 5:
                if (schedule_priority_preemptive == NULL) {
                    printf("[!] Priority (Preemptive) is not implemented yet.\n");
                    continue;
                }
                status = schedule_priority_preemptive(work, n, &metrics);
                break;

            case 6:
                if (schedule_rr == NULL) {
                    printf("[!] Round Robin is not implemented yet.\n");
                    continue;
                }
                if (!read_int("Enter Time Quantum: ", &quantum) || quantum <= 0) {
                    printf("[!] Invalid Time Quantum. Must be > 0.\n");
                    continue;
                }
                status = schedule_rr(work, n, quantum, &metrics);
                break;

            default:
                printf("[!] Invalid choice. Select 0-6.\n");
                continue;
        }

        if (status == 0) {
            print_results(work, n, &metrics);
        } else {
            printf("[!] Algorithm returned error status (%d).\n", status);
        }
    }

    free(orig);
    free(work);
    return 0;
}
