#include "scenario.h"

#include <stdlib.h>
#include <string.h>

static size_t cell(const Scenario *scenario, size_t process, size_t resource) {
    return process * scenario->resources + resource;
}

static void fill_standard(Scenario *scenario, ScenarioKind kind) {
    size_t process;
    size_t resource;

    for (resource = 0; resource < scenario->resources; ++resource) {
        scenario->available[resource] = kind == SCENARIO_SAFE ? (int)scenario->processes + 1 : (int)((scenario->processes + 1) / 2);
    }
    for (process = 0; process < scenario->processes; ++process) {
        for (resource = 0; resource < scenario->resources; ++resource) {
            size_t index = cell(scenario, process, resource);
            scenario->allocation[index] = (int)((process * 2 + resource) % 4 == 0);
            scenario->maximum[index] = scenario->allocation[index] + 1 + (int)((process + resource) % 3 == 0);
        }
    }
}

static void fill_deadlock(Scenario *scenario) {
    size_t process;
    size_t resource;

    for (resource = 0; resource < scenario->resources; ++resource) {
        scenario->available[resource] = 0;
    }
    for (process = 0; process < scenario->processes; ++process) {
        size_t held = process % scenario->resources;
        size_t wanted = (process + 1) % scenario->resources;
        scenario->allocation[cell(scenario, process, held)] = 1;
        scenario->maximum[cell(scenario, process, held)] = 1;
        scenario->maximum[cell(scenario, process, wanted)] = 1;
    }
}

int scenario_create(Scenario *scenario, size_t processes, size_t resources, ScenarioKind kind) {
    if (scenario == NULL || processes == 0 || resources == 0 || processes > 8 || resources > 5 || kind > SCENARIO_MIXED) {
        return 0;
    }
    memset(scenario, 0, sizeof(*scenario));
    scenario->processes = processes;
    scenario->resources = resources;
    scenario->available = calloc(resources, sizeof(*scenario->available));
    scenario->allocation = calloc(processes * resources, sizeof(*scenario->allocation));
    scenario->maximum = calloc(processes * resources, sizeof(*scenario->maximum));
    if (scenario->available == NULL || scenario->allocation == NULL || scenario->maximum == NULL) {
        scenario_destroy(scenario);
        return 0;
    }
    if (kind == SCENARIO_DEADLOCK && processes > 1) {
        fill_deadlock(scenario);
    } else {
        fill_standard(scenario, kind);
    }
    return 1;
}

void scenario_destroy(Scenario *scenario) {
    if (scenario == NULL) {
        return;
    }
    free(scenario->available);
    free(scenario->allocation);
    free(scenario->maximum);
    memset(scenario, 0, sizeof(*scenario));
}

BankerState scenario_state(const Scenario *scenario) {
    BankerState state = {0};
    if (scenario != NULL) {
        state.processes = scenario->processes;
        state.resources = scenario->resources;
        state.available = scenario->available;
        state.allocation = scenario->allocation;
        state.maximum = scenario->maximum;
    }
    return state;
}

const char *scenario_kind_name(ScenarioKind kind) {
    static const char *names[] = {"safe", "deadlock", "mixed"};
    return kind <= SCENARIO_MIXED ? names[kind] : "unknown";
}

int scenario_kind_parse(const char *name, ScenarioKind *kind) {
    if (name == NULL || kind == NULL) {
        return 0;
    }
    if (strcmp(name, "safe") == 0) {
        *kind = SCENARIO_SAFE;
    } else if (strcmp(name, "deadlock") == 0) {
        *kind = SCENARIO_DEADLOCK;
    } else if (strcmp(name, "mixed") == 0) {
        *kind = SCENARIO_MIXED;
    } else {
        return 0;
    }
    return 1;
}
