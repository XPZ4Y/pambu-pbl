#ifndef SCENARIO_H
#define SCENARIO_H

#include "banker.h"

typedef struct {
    size_t processes;
    size_t resources;
    int *available;
    int *allocation;
    int *maximum;
} Scenario;

typedef enum {
    SCENARIO_SAFE,
    SCENARIO_DEADLOCK,
    SCENARIO_MIXED
} ScenarioKind;

int scenario_create(Scenario *scenario, size_t processes, size_t resources, ScenarioKind kind);
void scenario_destroy(Scenario *scenario);
BankerState scenario_state(const Scenario *scenario);
const char *scenario_kind_name(ScenarioKind kind);
int scenario_kind_parse(const char *name, ScenarioKind *kind);

#endif
