#include "banker.h"

#include <stdlib.h>
#include <string.h>

static size_t cell(const BankerState *state, size_t process, size_t resource) {
    return process * state->resources + resource;
}

int banker_validate(const BankerState *state) {
    size_t process;
    size_t resource;

    if (state == NULL || state->processes == 0 || state->resources == 0 || state->available == NULL || state->allocation == NULL || state->maximum == NULL) {
        return 0;
    }
    for (resource = 0; resource < state->resources; ++resource) {
        if (state->available[resource] < 0) {
            return 0;
        }
    }
    for (process = 0; process < state->processes; ++process) {
        for (resource = 0; resource < state->resources; ++resource) {
            size_t index = cell(state, process, resource);
            if (state->allocation[index] < 0 || state->maximum[index] < state->allocation[index]) {
                return 0;
            }
        }
    }
    return 1;
}

void banker_result_destroy(BankerResult *result) {
    if (result == NULL) {
        return;
    }
    free(result->sequence);
    free(result->finished);
    memset(result, 0, sizeof(*result));
}

int banker_analyze(const BankerState *state, BankerResult *result) {
    int *work;
    size_t process;
    size_t resource;
    int progress;

    if (!banker_validate(state) || result == NULL) {
        return 0;
    }
    banker_result_destroy(result);
    work = malloc(state->resources * sizeof(*work));
    result->sequence = calloc(state->processes, sizeof(*result->sequence));
    result->finished = calloc(state->processes, sizeof(*result->finished));
    if (work == NULL || result->sequence == NULL || result->finished == NULL) {
        free(work);
        banker_result_destroy(result);
        return 0;
    }
    memcpy(work, state->available, state->resources * sizeof(*work));
    do {
        progress = 0;
        for (process = 0; process < state->processes; ++process) {
            int can_finish = !result->finished[process];
            for (resource = 0; can_finish && resource < state->resources; ++resource) {
                size_t index = cell(state, process, resource);
                if (state->maximum[index] - state->allocation[index] > work[resource]) {
                    can_finish = 0;
                }
            }
            if (can_finish) {
                for (resource = 0; resource < state->resources; ++resource) {
                    work[resource] += state->allocation[cell(state, process, resource)];
                }
                result->finished[process] = 1;
                result->sequence[result->completed++] = process;
                progress = 1;
            }
        }
    } while (progress);
    free(work);
    result->safe = result->completed == state->processes;
    return 1;
}

BankerRequestStatus banker_check_request(const BankerState *state, size_t process, const int *request, BankerResult *result) {
    int *available;
    int *allocation;
    int *maximum;
    BankerState candidate;
    size_t resource;

    if (!banker_validate(state) || request == NULL || process >= state->processes) {
        return BANKER_REQUEST_INVALID;
    }
    for (resource = 0; resource < state->resources; ++resource) {
        size_t index = cell(state, process, resource);
        if (request[resource] < 0) {
            return BANKER_REQUEST_INVALID;
        }
        if (request[resource] > state->maximum[index] - state->allocation[index]) {
            return BANKER_REQUEST_EXCEEDS_NEED;
        }
        if (request[resource] > state->available[resource]) {
            return BANKER_REQUEST_UNAVAILABLE;
        }
    }
    available = malloc(state->resources * sizeof(*available));
    allocation = malloc(state->processes * state->resources * sizeof(*allocation));
    maximum = malloc(state->processes * state->resources * sizeof(*maximum));
    if (available == NULL || allocation == NULL || maximum == NULL) {
        free(available);
        free(allocation);
        free(maximum);
        return BANKER_REQUEST_INVALID;
    }
    memcpy(available, state->available, state->resources * sizeof(*available));
    memcpy(allocation, state->allocation, state->processes * state->resources * sizeof(*allocation));
    memcpy(maximum, state->maximum, state->processes * state->resources * sizeof(*maximum));
    for (resource = 0; resource < state->resources; ++resource) {
        available[resource] -= request[resource];
        allocation[cell(state, process, resource)] += request[resource];
    }
    candidate = (BankerState){state->processes, state->resources, available, allocation, maximum};
    if (!banker_analyze(&candidate, result)) {
        free(available);
        free(allocation);
        free(maximum);
        return BANKER_REQUEST_INVALID;
    }
    free(available);
    free(allocation);
    free(maximum);
    return result->safe ? BANKER_REQUEST_GRANTED : BANKER_REQUEST_UNSAFE;
}

const char *banker_request_status_name(BankerRequestStatus status) {
    static const char *names[] = {"GRANTED", "EXCEEDS_NEED", "UNAVAILABLE", "UNSAFE", "INVALID"};
    return status >= BANKER_REQUEST_GRANTED && status <= BANKER_REQUEST_INVALID ? names[status] : "INVALID";
}
