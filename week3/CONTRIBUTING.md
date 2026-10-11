# CONTRIBUTING.md

This document gives the rules for contributing to the CPU Process Scheduling Simulator[cite: 1]. Read all steps before you write code.

---

## 1. Project Directory Structure

Make sure your local repository has this structure:

```text
project/
├── Makefile
├── include/
│   └── schedule.h          # Data structures and function contracts (DO NOT EDIT)
├── src/
│   ├── scheduler.c         # Main simulator CLI (DO NOT EDIT)
│   ├── fcfs.c              # First-Come-First-Served implementation
│   ├── sjf.c               # Shortest Job First implementation
│   ├── priority.c          # Priority Scheduling implementation
│   └── rr.c                # Round Robin implementation
├── test/
│   ├── test_fcfs.c         # Unit tests for FCFS
│   ├── test_sjf.c          # Unit tests for SJF
│   ├── test_priority.c     # Unit tests for Priority
│   └── test_rr.c           # Unit tests for Round Robin
└── report/
    ├── report_fcfs.md      # Analysis report for FCFS
    ├── report_sjf.md       # Analysis report for SJF
    ├── report_priority.md  # Analysis report for Priority
    └── report_rr.md        # Analysis report for Round Robin

```

---

## 2. How to View and Run Tests

Each scheduling algorithm in `src/` has a matching test file in `test/`.

### Step 1: Read the Interface Contract

Open `include/schedule.h`. This file defines the `Process`, `GanttSlice`, and `ScheduleMetrics` structures. It also defines the exact function names and parameters you must use.

### Step 2: Read Your Assigned Test File

Open the test file for your assigned algorithm:

* **FCFS:** `test/test_fcfs.c`

* **SJF (Preemptive and Non-Preemptive):** `test/test_sjf.c`

* **Priority (Preemptive and Non-Preemptive):** `test/test_priority.c`

* **Round Robin:** `test/test_rr.c`


Read the test cases inside your test file. Each test case shows the input processes, the expected Waiting Time (WT), Turnaround Time (TAT), Response Time (RT), and CPU utilization.

### Step 3: Run the Test Suite

1. Create your source file in `src/` (for example, `src/fcfs.c`).
2. Add `#include "schedule.h"` to the first line of your file.
3. Run the test command in your terminal:

```bash
make test

```

* If your function is missing, the test output shows: `[FAIL] '<function_name>' is not implemented.`
* If your calculation is wrong, the test output shows: `[FAIL] Assertion failed: ...`
* If your code is correct, the test output shows: `[PASS]` and `[RESULT] ALL TESTS PASSED`.

---

## 3. Coding Rules

1. **Include the header:** Every `.c` file in `src/` must start with `#include "schedule.h"`. The `Makefile` stops with an error if you omit this line.
2. **Do not change function signatures:** Your function name, return type, and parameter types must match `include/schedule.h` exactly.
3. **Validate inputs:** Return `-1` if `procs` is `NULL`, `metrics` is `NULL`, `n <= 0`, or `time_quantum <= 0`. Return `0` when the calculation succeeds.


4. **Follow priority and tie-breaking rules:**
* A lower priority integer means higher priority (for example, priority `1` runs before priority `2`).
* If two processes have the same metric, run the process with the earlier `arrival_time`. If `arrival_time` is also equal, run the process with the lower `pid`.




5. **Record idle time:** If the CPU has no ready processes during a time interval, record a `GanttSlice` with `pid = -1`.



---

## 4. How to Use AI to Generate Your Code

You can use an AI assistant to generate the C code for your algorithm. To make sure the AI code compiles and passes all tests on the first try, give the AI the exact header contract and test file.

### Step 1: Copy the Prompt Template

Copy the prompt below and paste it into your AI tool. Replace `<algo>` with your assigned algorithm name (`fcfs`, `sjf`, `priority`, or `rr`).

```text
Write the C implementation for `src/<algo>.c` in C11.

Follow these strict rules:
1. Start the file with `#include "schedule.h"`. Do not redefine the structs in `schedule.h`.
2. Implement ONLY the function(s) declared for `src/<algo>.c` in `schedule.h`.
3. Do not include a `main()` function.
4. Return `-1` for invalid inputs (`procs == NULL`, `metrics == NULL`, `n <= 0`, or `time_quantum <= 0`). Return `0` on success.
5. Lower priority numbers indicate higher priority. Break ties by earlier `arrival_time`, then by lower `pid`.
6. Populate all fields for each `Process` (`start_time`, `completion_time`, `turnaround_time`, `waiting_time`, `response_time`) and all fields in `ScheduleMetrics` (`avg_waiting_time`, `avg_turnaround_time`, `avg_response_time`, `cpu_utilization`, `gantt`, `gantt_len`).
7. Merge consecutive execution ticks of the same process (or `-1` for CPU idle time) into a single `GanttSlice`.

Here is `include/schedule.h`:
[PASTE FULL CONTENT OF include/schedule.h HERE]

Here is `test/test_<algo>.c` that your code must pass:
[PASTE FULL CONTENT OF test/test_<algo>.c HERE]

```

### Step 2: Verify and Debug AI Output

1. Save the generated code into `src/<algo>.c`.
2. Run `make clean && make test`.
3. If a test fails, copy the terminal error message and paste it back into the AI tool. Ask the AI to fix the calculation that caused the assertion failure.
4. Run `make run` to test your algorithm interactively in the main simulator.



---

## 5. How to Write Your Algorithm Report in `/report`

After your code passes `make test`, you must document your results. Create a Markdown file in the `report/` directory named `report_<algo>.md` (for example, `report/report_fcfs.md`).

Your report must include these 4 sections:

### 1. Algorithm Overview

Explain how your scheduling algorithm selects the next process. State whether the algorithm is preemptive or non-preemptive, and explain how it handles CPU idle time and tie-breaking.

### 2. Three Test Cases and Gantt Charts

Run your algorithm using `make run` on at least **three different test cases**:

* **Test Case 1:** Processes that arrive at different times with no CPU idle gaps.
* **Test Case 2:** Processes that cause a CPU idle gap (for example, `P1` finishes at `t = 2`, and `P2` arrives at `t = 5`).
* **Test Case 3:** Edge case (for example, all processes arrive at `t = 0`, or processes have identical burst times/priorities).



For each test case, copy and paste the terminal output from `make run`, including:

* Input table (`PID`, `AT`, `BT`, `PRIO`, and `Time Quantum` if applicable)


* The ASCII Gantt Chart


* Output metrics (`CT`, `TAT`, `WT`, `RT`, `Average WT`, `Average TAT`, `Average RT`, and `CPU Utilization`)



### 3. Performance Observations

Analyze the data from your three test cases:

* Explain how burst time, arrival time, priority, or time quantum changes the Average Waiting Time and Average Turnaround Time.


* Identify known limitations observed in your tests (for example, the **convoy effect** in FCFS, **starvation** in SJF/Priority, or **context-switch overhead** in Round Robin with a small time quantum).

### 4. Complexity Analysis

State the time complexity and space complexity of your implementation using Big-O notation (for example, $\mathcal{O}(n^2)$ time and $\mathcal{O}(n)$ space).

---

## 6. Final Submission Checklist

Before you open a Pull Request, run these commands and verify every item:

* [ ] `make clean && make test` compiles with zero warnings and prints `Suite status: ALL PASSED`.
* [ ] `make app` links your algorithm into `build/bin/scheduler` without errors.
* [ ] Your `src/<algo>.c` file includes `"schedule.h"` and has no unused variables.
* [ ] Your `report/report_<algo>.md` file exists and contains 3 test cases, Gantt charts, and performance analysis.




