#include "orchestrator.h"

/* Validate simulation input. */
int validate_input(const SimInput *in)
{
    if (in == NULL)
        return 0;

    if (in->ref_count < 1 || in->ref_count > MAX_REFERENCES)
        return 0;

    if (in->frame_count < 1 || in->frame_count > MAX_FRAMES)
        return 0;

    if (in->choice != ALGO_FIFO &&
        in->choice != ALGO_LRU &&
        in->choice != ALGO_OPT &&
        in->choice != ALGO_ALL)
        return 0;

    for (int i = 0; i < in->ref_count; i++) {
        if (in->refs[i] < 0)
            return 0;
    }

    return 1;
}

/* Run the selected page-replacement algorithm(s). */
void run_simulation(const SimInput *in,
                    SimResult results[],
                    int *result_count)
{
    if (result_count == NULL)
        return;

    *result_count = 0;

    if (in == NULL || results == NULL || !validate_input(in))
        return;

    AlgoInput algo_in = {0};
    algo_in.ref_count = in->ref_count;
    algo_in.frame_count = in->frame_count;

    for (int i = 0; i < in->ref_count; i++)
        algo_in.refs[i] = in->refs[i];

    if (in->choice == ALGO_ALL) {
        simulate_fifo(&algo_in, &results[0]);
        simulate_lru(&algo_in, &results[1]);
        simulate_optimal(&algo_in, &results[2]);
        *result_count = 3;
        return;
    }

    switch (in->choice) {
    case ALGO_FIFO:
        simulate_fifo(&algo_in, &results[0]);
        break;
    case ALGO_LRU:
        simulate_lru(&algo_in, &results[0]);
        break;
    case ALGO_OPT:
        simulate_optimal(&algo_in, &results[0]);
        break;
    default:
        return;
    }

    *result_count = 1;
}

/* Find the algorithm with the fewest page faults.
   Ties go to the lowest index. Invalid results are skipped. */
int find_best_algorithm(const SimResult results[], int result_count)
{
    if (results == NULL || result_count <= 0)
        return -1;

    int best = -1;

    for (int i = 0; i < result_count; i++) {
        if (results[i].faults < 0)
            continue;

        if (best == -1 || results[i].faults < results[best].faults)
            best = i;
    }

    return best;
}
