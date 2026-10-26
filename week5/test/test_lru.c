/* test/test_lru.c — LRU simulation tests. */
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

/* Classic 3-frame LRU on 1..12 sequence → 10 faults / 2 hits. */
static void test_classic_3frame(void) {
    const int refs[] = {1,2,3,4,1,2,5,1,2,3,4,5};
    AlgoInput in = mk(refs, 12, 3);
    SimResult r; simulate_lru(&in, &r);
    CHECK(r.algo == ALGO_LRU);
    CHECK(r.frames == 3);
    CHECK(r.total_refs == 12);
    CHECK(r.step_count == 12);
    CHECK(r.hits == 2);
    CHECK(r.faults == 10);
    CHECK(r.hits + r.faults == r.total_refs);
    if (r.name) CHECK(strcmp(r.name, "LRU") == 0);
}

static void test_all_hits(void) {
    const int refs[] = {1,2,3,1,2,3};
    AlgoInput in = mk(refs, 6, 3);
    SimResult r; simulate_lru(&in, &r);
    CHECK(r.faults == 0);
    CHECK(r.hits == 6);
}

static void test_one_frame(void) {
    const int refs[] = {1,2,3,1};
    AlgoInput in = mk(refs, 4, 1);
    SimResult r; simulate_lru(&in, &r);
    CHECK(r.faults == 4);
    CHECK(r.hits == 0);
}

/* LRU must evict the least-recently-used — verify a known trace:
 * frames=2, refs=1,2,1,3 → at ref 3, page 2 is LRU and gets evicted. */
static void test_lru_eviction(void) {
    const int refs[] = {1,2,1,3};
    AlgoInput in = mk(refs, 4, 2);
    SimResult r; simulate_lru(&in, &r);
    CHECK(r.faults == 3);        /* 1F, 2F, 1H, 3F */
    CHECK(r.hits == 1);
    /* After step 3 (ref=3), frame containing 2 must be gone. */
    int seen1 = 0, seen2 = 0, seen3 = 0;
    for (int f = 0; f < r.frames; ++f) {
        if (r.steps[3].frames[f] == 1) seen1 = 1;
        if (r.steps[3].frames[f] == 2) seen2 = 1;
        if (r.steps[3].frames[f] == 3) seen3 = 1;
    }
    CHECK(seen1 && seen3);
    CHECK(!seen2);
    CHECK(r.steps[3].victim_frame >= 0);
}

static void test_minimal(void) {
    const int refs[] = {7};
    AlgoInput in = mk(refs, 1, 1);
    SimResult r; simulate_lru(&in, &r);
    CHECK(r.faults == 1);
    CHECK(r.steps[0].victim_frame == -1);
}

int main(void) {
    test_classic_3frame();
    test_all_hits();
    test_one_frame();
    test_lru_eviction();
    test_minimal();
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("test_lru: PASS\n");
    return 0;
}
