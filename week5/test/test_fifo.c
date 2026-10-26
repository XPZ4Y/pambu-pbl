/* test/test_fifo.c — FIFO simulation tests.
 * Links against the real core (no stubs); RED until fifo.c lands. */
#include "orchestrator.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } \
} while (0)

static AlgoInput mk(const int *refs, int n, int frames) {
    AlgoInput in; memset(&in, 0, sizeof in);
    for (int i = 0; i < n; ++i) in.refs[i] = refs[i];
    in.ref_count = n; in.frame_count = frames;
    return in;
}

/* Classic 3-frame FIFO: 1,2,3,4,1,2,5,1,2,3,4,5 → 9 faults / 3 hits */
static void test_classic_3frame(void) {
    const int refs[] = {1,2,3,4,1,2,5,1,2,3,4,5};
    AlgoInput in = mk(refs, 12, 3);
    SimResult r; simulate_fifo(&in, &r);
    CHECK(r.algo == ALGO_FIFO);
    CHECK(r.frames == 3);
    CHECK(r.total_refs == 12);
    CHECK(r.step_count == 12);
    CHECK(r.hits == 3);
    CHECK(r.faults == 9);
    CHECK(r.hits + r.faults == r.total_refs);
    if (r.name) CHECK(strcmp(r.name, "FIFO") == 0);
}

/* No faults when all refs fit in frames. */
static void test_all_hits(void) {
    const int refs[] = {1,2,3,1,2,3};
    AlgoInput in = mk(refs, 6, 3);
    SimResult r; simulate_fifo(&in, &r);
    CHECK(r.faults == 0);
    CHECK(r.hits == 6);
}

/* Every ref is a fault when frames==1. */
static void test_one_frame(void) {
    const int refs[] = {1,2,3,1};
    AlgoInput in = mk(refs, 4, 1);
    SimResult r; simulate_fifo(&in, &r);
    CHECK(r.faults == 4);
    CHECK(r.hits == 0);
}

/* Single reference, single frame. */
static void test_minimal(void) {
    const int refs[] = {7};
    AlgoInput in = mk(refs, 1, 1);
    SimResult r; simulate_fifo(&in, &r);
    CHECK(r.faults == 1);
    CHECK(r.step_count == 1);
    CHECK(r.steps[0].status == STATUS_FAULT);
    CHECK(r.steps[0].victim_frame == -1);   /* first fill, no eviction */
}

/* Victim frame is -1 on a hit; on an eviction it is a valid frame index. */
static void test_victim_semantics(void) {
    const int refs[] = {1,1,2,2,3};   /* frames=2: 1F 1H 2F 2H 3F(victim) */
    AlgoInput in = mk(refs, 5, 2);
    SimResult r; simulate_fifo(&in, &r);
    CHECK(r.steps[1].status == STATUS_HIT);
    CHECK(r.steps[1].victim_frame == -1);
    CHECK(r.steps[3].status == STATUS_HIT);
    CHECK(r.steps[3].victim_frame == -1);
    CHECK(r.steps[4].status == STATUS_FAULT);
    CHECK(r.steps[4].victim_frame >= 0 && r.steps[4].victim_frame < 2);
}

/* Empty slots are EMPTY_FRAME. */
static void test_empty_slots(void) {
    const int refs[] = {1};
    AlgoInput in = mk(refs, 1, 3);
    SimResult r; simulate_fifo(&in, &r);
    int empties = 0;
    for (int f = 0; f < r.frames; ++f)
        if (r.steps[0].frames[f] == EMPTY_FRAME) ++empties;
    CHECK(empties == 2);
}

int main(void) {
    test_classic_3frame();
    test_all_hits();
    test_one_frame();
    test_minimal();
    test_victim_semantics();
    test_empty_slots();
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("test_fifo: PASS\n");
    return 0;
}
