/* test/test_optimal.c — Optimal (Belady) simulation tests. */
#include "orchestrator.h"

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

/* Belady's classic: 3 frames, 1..12 → 7 faults (best possible). */
static void test_classic_3frame(void) {
    const int refs[] = {1,2,3,4,1,2,5,1,2,3,4,5};
    AlgoInput in = mk(refs, 12, 3);
    SimResult r; simulate_optimal(&in, &r);
    CHECK(r.algo == ALGO_OPT);
    CHECK(r.frames == 3);
    CHECK(r.total_refs == 12);
    CHECK(r.step_count == 12);
    CHECK(r.faults == 7);
    CHECK(r.hits == 5);
    CHECK(r.hits + r.faults == r.total_refs);
    if (r.name) CHECK(strcmp(r.name, "Optimal") == 0);
}

static void test_all_hits(void) {
    const int refs[] = {1,2,3,1,2,3};
    AlgoInput in = mk(refs, 6, 3);
    SimResult r; simulate_optimal(&in, &r);
    CHECK(r.faults == 0);
    CHECK(r.hits == 6);
}

static void test_one_frame(void) {
    const int refs[] = {1,2,3,1};
    AlgoInput in = mk(refs, 4, 1);
    SimResult r; simulate_optimal(&in, &r);
    CHECK(r.faults == 4);
    CHECK(r.hits == 0);
}

/* Optimal evicts the page used farthest in the future. refs=1,2,3,1,4
 * with 3 frames: at ref=4, page 1 (next use never), page 2 (never),
 * page 3 (never) — any victim is valid, but 4 must fault. */
static void test_far_future_eviction(void) {
    const int refs[] = {1,2,3,4,1,2};
    AlgoInput in = mk(refs, 6, 3);
    SimResult r; simulate_optimal(&in, &r);
    /* Optimal: 4 faults (1,2,3,4), then hits on 1,2 → faults=4 hits=2 */
    CHECK(r.faults == 4);
    CHECK(r.hits == 2);
}

static void test_minimal(void) {
    const int refs[] = {7};
    AlgoInput in = mk(refs, 1, 1);
    SimResult r; simulate_optimal(&in, &r);
    CHECK(r.faults == 1);
    CHECK(r.steps[0].victim_frame == -1);
}

int main(void) {
    test_classic_3frame();
    test_all_hits();
    test_one_frame();
    test_far_future_eviction();
    test_minimal();
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("test_optimal: PASS\n");
    return 0;
}
