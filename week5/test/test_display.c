/* test/test_display.c — result_init + printing helpers.
 *
 * Printing tests capture stdout via a pipe so we can assert substring
 * content. display.c uses printf() per the header (no FILE* param). */
#include "orchestrator.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } \
} while (0)

/* Capture stdout produced by `fn(arg)` into `out`. Truncates at cap. */
typedef void (*CaptureFn)(void *arg);
static void capture(CaptureFn fn, void *arg, char *out, size_t cap) {
    fflush(stdout);
    int pfd[2];
    if (pipe(pfd) != 0) { out[0] = '\0'; return; }
    int saved = dup(STDOUT_FILENO);
    dup2(pfd[1], STDOUT_FILENO);
    close(pfd[1]);
    fn(arg);
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
    ssize_t n = read(pfd[0], out, cap - 1);
    if (n < 0) n = 0;
    out[n] = '\0';
    close(pfd[0]);
}

static void test_result_init(void) {
    AlgoInput in; memset(&in, 0, sizeof in);
    in.ref_count = 10; in.frame_count = 3;
    SimResult r;
    result_init(&r, "FIFO", ALGO_FIFO, &in);
    CHECK(r.name != NULL && strcmp(r.name, "FIFO") == 0);
    CHECK(r.algo == ALGO_FIFO);
    CHECK(r.frames == 3);
    CHECK(r.total_refs == 10);
    CHECK(r.step_count == 0);
    CHECK(r.hits == 0);
    CHECK(r.faults == 0);
}

static void call_print_header(void *a) { print_header((const char *)a); }
static void test_print_header(void) {
    char buf[512];
    capture(call_print_header, (void *)"MyTitle", buf, sizeof buf);
    CHECK(strstr(buf, "MyTitle") != NULL);
}

static SimResult g_r;
static void call_print_sim_table(void *a) { (void)a; print_sim_table(&g_r); }
static void call_print_stats(void *a)    { (void)a; print_stats(&g_r); }
static void call_print_cmp(void *a)      { (void)a; print_comparison_table((SimResult *)a, 3); }

static void test_print_sim_table(void) {
    /* Build a small trace: two faults, one hit. */
    memset(&g_r, 0, sizeof g_r);
    g_r.name = "FIFO"; g_r.algo = ALGO_FIFO; g_r.frames = 2;
    g_r.total_refs = 3; g_r.hits = 1; g_r.faults = 2; g_r.step_count = 3;
    for (int i = 0; i < 3; ++i) {
        g_r.steps[i].frame_count = 2;
        for (int f = 0; f < 2; ++f) g_r.steps[i].frames[f] = EMPTY_FRAME;
        g_r.steps[i].victim_frame = -1;
    }
    g_r.steps[0].page = 1; g_r.steps[0].status = STATUS_FAULT; g_r.steps[0].frames[0] = 1;
    g_r.steps[1].page = 2; g_r.steps[1].status = STATUS_FAULT; g_r.steps[1].frames[1] = 2;
    g_r.steps[2].page = 1; g_r.steps[2].status = STATUS_HIT;
    char buf[4096];
    capture(call_print_sim_table, NULL, buf, sizeof buf);
    CHECK(strlen(buf) > 0);   /* non-empty; content varies by format */
}

static void test_print_stats(void) {
    memset(&g_r, 0, sizeof g_r);
    g_r.name = "FIFO"; g_r.algo = ALGO_FIFO; g_r.frames = 3;
    g_r.total_refs = 12; g_r.hits = 3; g_r.faults = 9;
    char buf[2048];
    capture(call_print_stats, NULL, buf, sizeof buf);
    CHECK(strstr(buf, "9") != NULL);       /* faults value printed */
    CHECK(strstr(buf, "3") != NULL);       /* hits or frames value */
}

static void test_print_comparison_table(void) {
    SimResult r[3]; memset(r, 0, sizeof r);
    r[0].name = "FIFO";    r[0].algo = ALGO_FIFO; r[0].faults = 9; r[0].hits = 3;
    r[1].name = "LRU";     r[1].algo = ALGO_LRU;  r[1].faults = 10; r[1].hits = 2;
    r[2].name = "Optimal"; r[2].algo = ALGO_OPT;  r[2].faults = 7; r[2].hits = 5;
    char buf[4096];
    capture(call_print_cmp, r, buf, sizeof buf);
    CHECK(strstr(buf, "FIFO") != NULL);
    CHECK(strstr(buf, "LRU") != NULL);
    CHECK(strstr(buf, "Optimal") != NULL);
}

int main(void) {
    test_result_init();
    test_print_header();
    test_print_sim_table();
    test_print_stats();
    test_print_comparison_table();
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("test_display: PASS\n");
    return 0;
}
