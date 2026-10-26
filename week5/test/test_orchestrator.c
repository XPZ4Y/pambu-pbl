/* test/test_orchestrator.c — validate_input, run_simulation dispatch,
 * find_best_algorithm. Links against real core. */
#include "orchestrator.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } \
} while (0)

static SimInput mk(int n, int frames, AlgorithmChoice c) {
    SimInput in; memset(&in, 0, sizeof in);
    for (int i = 0; i < n; ++i) in.refs[i] = i % 7;
    in.ref_count = n; in.frame_count = frames; in.choice = c;
    return in;
}

static void test_validate_ok(void) {
    SimInput in = mk(5, 3, ALGO_FIFO);
    CHECK(validate_input(&in) == 1);
    in.choice = ALGO_LRU;  CHECK(validate_input(&in) == 1);
    in.choice = ALGO_OPT;  CHECK(validate_input(&in) == 1);
    in.choice = ALGO_ALL;  CHECK(validate_input(&in) == 1);
}

static void test_validate_bounds(void) {
    SimInput in = mk(1, 1, ALGO_FIFO);
    CHECK(validate_input(&in) == 1);

    /* ref_count boundaries */
    in.ref_count = 0;                  CHECK(validate_input(&in) == 0);
    in.ref_count = MAX_REFERENCES;     CHECK(validate_input(&in) == 1);
    in.ref_count = MAX_REFERENCES + 1; CHECK(validate_input(&in) == 0);

    /* frame_count boundaries */
    in.ref_count = 5;
    in.frame_count = 0;                CHECK(validate_input(&in) == 0);
    in.frame_count = MAX_FRAMES;       CHECK(validate_input(&in) == 1);
    in.frame_count = MAX_FRAMES + 1;   CHECK(validate_input(&in) == 0);

    /* choice boundaries */
    in.frame_count = 3;
    in.choice = (AlgorithmChoice)0;    CHECK(validate_input(&in) == 0);
    in.choice = (AlgorithmChoice)5;    CHECK(validate_input(&in) == 0);

    /* negative ref */
    in.choice = ALGO_FIFO;
    in.refs[0] = -1;                   CHECK(validate_input(&in) == 0);
}

static void test_validate_null(void) {
    CHECK(validate_input(NULL) == 0);
}

static void test_run_simulation_single(void) {
    SimInput in = mk(10, 3, ALGO_FIFO);
    SimResult r[3]; int n = 0;
    run_simulation(&in, r, &n);
    CHECK(n == 1);
    CHECK(r[0].algo == ALGO_FIFO);
    CHECK(r[0].step_count == 10);
    CHECK(r[0].hits + r[0].faults == 10);
}

static void test_run_simulation_all(void) {
    SimInput in = mk(12, 3, ALGO_ALL);
    SimResult r[3]; int n = 0;
    run_simulation(&in, r, &n);
    CHECK(n == 3);
    if (n == 3) {
        CHECK(r[0].algo == ALGO_FIFO);
        CHECK(r[1].algo == ALGO_LRU);
        CHECK(r[2].algo == ALGO_OPT);
        for (int i = 0; i < 3; ++i)
            CHECK(r[i].hits + r[i].faults == 12);
    }
}

static void test_find_best_lowest(void) {
    /* Synthetic: FIFO=9, LRU=10, OPT=7 → OPT wins (index 2). */
    SimResult r[3]; memset(r, 0, sizeof r);
    r[0].algo = ALGO_FIFO; r[0].faults = 9;
    r[1].algo = ALGO_LRU;  r[1].faults = 10;
    r[2].algo = ALGO_OPT;  r[2].faults = 7;
    CHECK(find_best_algorithm(r, 3) == 2);
}

static void test_find_best_tie_lowest_index(void) {
    /* Tie on faults=5 → lowest index (0) wins by our assumption. */
    SimResult r[3]; memset(r, 0, sizeof r);
    r[0].faults = 5; r[1].faults = 5; r[2].faults = 5;
    CHECK(find_best_algorithm(r, 3) == 0);
}

static void test_find_best_single(void) {
    SimResult r[1]; memset(r, 0, sizeof r);
    r[0].faults = 3;
    CHECK(find_best_algorithm(r, 1) == 0);
    CHECK(find_best_algorithm(NULL, 0) == -1);
    CHECK(find_best_algorithm(r, 0) == -1);
}

static void test_find_best_negatives(void) {
    /* If some results have faults=-1 (uninitialized), they should not win. */
    SimResult r[3]; memset(r, 0, sizeof r);
    r[0].faults = 9;
    r[1].faults = -1;
    r[2].faults = 8;
    /* Interpretation may vary; we only assert the winner is not -1. */
    int b = find_best_algorithm(r, 3);
    CHECK(b >= 0);
    CHECK(r[b].faults != -1);
}

int main(void) {
    test_validate_ok();
    test_validate_bounds();
    test_validate_null();
    test_run_simulation_single();
    test_run_simulation_all();
    test_find_best_lowest();
    test_find_best_tie_lowest_index();
    test_find_best_single();
    test_find_best_negatives();
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("test_orchestrator: PASS\n");
    return 0;
}
