#include "banker.h"
#include "scenario.h"

#include <stdio.h>
#include <stdlib.h>

static int parse_size(const char *text, size_t minimum, size_t maximum, size_t *value) {
    char *end;
    unsigned long parsed = strtoul(text, &end, 10);
    if (*text == '\0' || *end != '\0' || parsed < minimum || parsed > maximum) {
        return 0;
    }
    *value = (size_t)parsed;
    return 1;
}

static void print_vector(const int *values, size_t count) {
    size_t index;
    for (index = 0; index < count; ++index) {
        printf("%s%d", index == 0 ? "" : " ", values[index]);
    }
}

int main(int argc, char **argv) {
    Scenario scenario = {0};
    BankerState state;
    BankerResult result = {0};
    ScenarioKind kind = SCENARIO_SAFE;
    size_t processes = 4;
    size_t resources = 3;
    size_t index;

    if (argc > 1 && !scenario_kind_parse(argv[1], &kind)) {
        fprintf(stderr, "Usage: %s [safe|deadlock|mixed] [processes 1-8] [resources 1-5]\n", argv[0]);
        return 2;
    }
    if ((argc > 2 && !parse_size(argv[2], 1, 8, &processes)) || (argc > 3 && !parse_size(argv[3], 1, 5, &resources)) || argc > 4) {
        fprintf(stderr, "Usage: %s [safe|deadlock|mixed] [processes 1-8] [resources 1-5]\n", argv[0]);
        return 2;
    }
    if (!scenario_create(&scenario, processes, resources, kind)) {
        fprintf(stderr, "Unable to create scenario.\n");
        return 1;
    }
    state = scenario_state(&scenario);
    if (!banker_analyze(&state, &result)) {
        fprintf(stderr, "Unable to analyze scenario.\n");
        scenario_destroy(&scenario);
        return 1;
    }
    printf("Scenario: %s\n", scenario_kind_name(kind));
    printf("Available: ");
    print_vector(state.available, state.resources);
    printf("\nState: %s\n", result.safe ? "SAFE" : "UNSAFE");
    printf("Completed: %zu/%zu\n", result.completed, state.processes);
    printf("Sequence:");
    for (index = 0; index < result.completed; ++index) {
        printf("%sP%zu", index == 0 ? " " : " -> ", result.sequence[index]);
    }
    printf("\n");
    int status;

    status = result.safe ? 0 : 3;
    banker_result_destroy(&result);
    scenario_destroy(&scenario);
    return status;
}
