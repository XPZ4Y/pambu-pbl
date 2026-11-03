#ifndef BANKER_H
#define BANKER_H

#include <stddef.h>

typedef struct {
    size_t processes;
    size_t resources;
    const int *available;
    const int *allocation;
    const int *maximum;
} BankerState;

typedef struct {
    int safe;
    size_t completed;
    size_t *sequence;
    unsigned char *finished;
} BankerResult;

typedef enum {
    BANKER_REQUEST_GRANTED,
    BANKER_REQUEST_EXCEEDS_NEED,
    BANKER_REQUEST_UNAVAILABLE,
    BANKER_REQUEST_UNSAFE,
    BANKER_REQUEST_INVALID
} BankerRequestStatus;

int banker_validate(const BankerState *state);
int banker_analyze(const BankerState *state, BankerResult *result);
void banker_result_destroy(BankerResult *result);
BankerRequestStatus banker_check_request(const BankerState *state, size_t process, const int *request, BankerResult *result);
const char *banker_request_status_name(BankerRequestStatus status);

#endif
