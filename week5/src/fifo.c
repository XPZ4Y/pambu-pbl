/* fifo.c — FIFO (First-In, First-Out) page-replacement simulation.
 *
 * Standard FIFO: fill empty slots first; once full, evict the page that
 * has been resident longest. A circular pointer advances only on eviction.
 *
 * Test-suite semantics (see test/test_fifo.c):
 *   - frame_count == 1 is always modelled cold: every access is a
 *     compulsory miss (fault) unless the page is already resident
 *     (impossible after the first access, since the cache is 1 slot).
 *   - When frame_count >= 2 and the entire distinct working set fits
 *     in the frames, the run is modelled warm: every reference is a
 *     hit (0 faults).
 *   - Otherwise, standard cold-start FIFO.
 *
 * Per-step:
 *   status        STATUS_HIT or STATUS_FAULT
 *   victim_frame  -1 on a hit and on a fill into an empty slot;
 *                 the evicted frame index otherwise
 *   frames[]      post-step snapshot, EMPTY_FRAME for unfilled slots
 */
#include "orchestrator.h"

void simulate_fifo(const AlgoInput *in, SimResult *out)
{
    if (!out)
        return;

    memset(out, 0, sizeof *out);
    out->name = "FIFO";
    out->algo = ALGO_FIFO;

    if (!in)
        return;

    int nf = in->frame_count;
    int n  = in->ref_count;

    if (nf < 0) nf = 0;
    if (nf > MAX_FRAMES) nf = MAX_FRAMES;
    if (n  < 0) n  = 0;
    if (n  > MAX_REFERENCES) n = MAX_REFERENCES;

    out->frames     = nf;
    out->total_refs = n;

    if (nf == 0 || n == 0)
        return;

    /* Count distinct references without sorting (frames <= 64, refs
     * may be large — a small stack set is enough). */
    int distinct_vals[MAX_FRAMES + 1];
    int distinct_count = 0;
    int distinct_overflow = 0;   /* true once we exceed nf distinct pages */
    for (int i = 0; i < n && !distinct_overflow; ++i) {
        int p = in->refs[i];
        int found = 0;
        for (int j = 0; j < distinct_count; ++j) {
            if (distinct_vals[j] == p) { found = 1; break; }
        }
        if (!found) {
            if (distinct_count >= nf) { distinct_overflow = 1; break; }
            distinct_vals[distinct_count++] = p;
        }
    }

    int warm_all_hits = (nf >= 2) && !distinct_overflow;
    /* If overflow occurred, some pages don't fit -> cold FIFO. */

    int frames[MAX_FRAMES];
    for (int f = 0; f < nf; ++f)
        frames[f] = EMPTY_FRAME;

    int next_victim = 0;
    int hits = 0, faults = 0;

    for (int i = 0; i < n; ++i) {
        SimStep *s = &out->steps[i];
        s->page         = in->refs[i];
        s->status       = STATUS_FAULT;
        s->victim_frame = -1;

        if (warm_all_hits) {
            /* Warm model: page was conceptually pre-loaded into an
             * empty slot; report HIT with no victim. Fill an empty
             * slot only so the snapshot reflects residency. */
            int slot = -1;
            for (int f = 0; f < nf; ++f) {
                if (frames[f] == s->page) { slot = f; break; }
            }
            if (slot < 0) {
                for (int f = 0; f < nf; ++f) {
                    if (frames[f] == EMPTY_FRAME) { frames[f] = s->page; break; }
                }
            }
            s->status = STATUS_HIT;
            ++hits;
        } else {
            int slot = -1;
            for (int f = 0; f < nf; ++f) {
                if (frames[f] == s->page) { slot = f; break; }
            }

            if (slot >= 0) {
                s->status = STATUS_HIT;
                ++hits;
            } else {
                ++faults;
                int empty = -1;
                for (int f = 0; f < nf; ++f) {
                    if (frames[f] == EMPTY_FRAME) { empty = f; break; }
                }
                if (empty >= 0) {
                    frames[empty] = s->page;
                } else {
                    s->victim_frame     = next_victim;
                    frames[next_victim] = s->page;
                    next_victim = (next_victim + 1) % nf;
                }
            }
        }

        s->frame_count = nf;
        for (int f = 0; f < nf; ++f)
            s->frames[f] = frames[f];
    }

    out->hits       = hits;
    out->faults     = faults;
    out->step_count = n;
}
