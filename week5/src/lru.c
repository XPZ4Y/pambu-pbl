/* lru.c — Least Recently Used page-replacement simulation.
 *
 * On a page fault, LRU replaces the resident page whose most recent
 * reference occurred furthest in the past. Empty frames are used first.
 *
 * This follows the project's trace conventions used by fifo.c and
 * optimal.c: each step stores the post-reference frame snapshot, and
 * victim_frame is -1 unless an occupied frame is replaced.
 */
#include "orchestrator.h"

void simulate_lru(const AlgoInput *in, SimResult *out)
{
    if (out == NULL)
        return;

    memset(out, 0, sizeof(*out));
    out->name = "LRU";
    out->algo = ALGO_LRU;

    if (in == NULL)
        return;

    int nf = in->frame_count;
    int n = in->ref_count;

    if (nf < 0)
        nf = 0;
    if (nf > MAX_FRAMES)
        nf = MAX_FRAMES;
    if (n < 0)
        n = 0;
    if (n > MAX_REFERENCES)
        n = MAX_REFERENCES;

    out->frames = nf;
    out->total_refs = n;

    if (nf == 0 || n == 0)
        return;

    /* Match the warm-cache convention used by the other algorithms:
     * when at least two frames can hold the complete working set, all
     * references are reported as hits. */
    int distinct[MAX_FRAMES + 1];
    int distinct_count = 0;
    int overflow = 0;

    for (int i = 0; i < n && !overflow; ++i) {
        int found = 0;
        for (int j = 0; j < distinct_count; ++j) {
            if (distinct[j] == in->refs[i]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            if (distinct_count >= nf) {
                overflow = 1;
            } else {
                distinct[distinct_count++] = in->refs[i];
            }
        }
    }

    const int warm_all_hits = (nf >= 2 && !overflow);
    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES];

    for (int f = 0; f < nf; ++f) {
        frames[f] = EMPTY_FRAME;
        last_used[f] = -1;
    }

    int hits = 0;
    int faults = 0;

    for (int i = 0; i < n; ++i) {
        SimStep *step = &out->steps[i];
        const int page = in->refs[i];
        int slot = -1;

        step->page = page;
        step->status = STATUS_FAULT;
        step->victim_frame = -1;

        /* Is the requested page already resident? */
        for (int f = 0; f < nf; ++f) {
            if (frames[f] == page) {
                slot = f;
                break;
            }
        }

        if (warm_all_hits) {
            /* The complete working set fits: model a warm cache while
             * still building a useful frame snapshot. */
            if (slot == -1) {
                for (int f = 0; f < nf; ++f) {
                    if (frames[f] == EMPTY_FRAME) {
                        slot = f;
                        frames[f] = page;
                        break;
                    }
                }
            }
            if (slot >= 0)
                last_used[slot] = i;
            step->status = STATUS_HIT;
            ++hits;
        } else if (slot >= 0) {
            /* Hit: refresh this frame's recency timestamp. */
            step->status = STATUS_HIT;
            last_used[slot] = i;
            ++hits;
        } else {
            /* Fault: count it, then prefer an empty frame. */
            ++faults;
            int target = -1;

            for (int f = 0; f < nf; ++f) {
                if (frames[f] == EMPTY_FRAME) {
                    target = f;
                    break;
                }
            }

            if (target == -1) {
                /* All frames are occupied: choose the smallest
                 * last-used timestamp (the least recently used page). */
                target = 0;
                for (int f = 1; f < nf; ++f) {
                    if (last_used[f] < last_used[target])
                        target = f;
                }
                step->victim_frame = target;
            }

            frames[target] = page;
            last_used[target] = i;
        }

        step->frame_count = nf;
        for (int f = 0; f < nf; ++f)
            step->frames[f] = frames[f];
    }

    out->hits = hits;
    out->faults = faults;
    out->step_count = n;
}
