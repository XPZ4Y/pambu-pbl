#include "banker.h"
#include "scenario.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void expect(int condition, const char *name) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        ++failures;
    }
}

static void test_safe_scenario(void) {
    Scenario scenario = {0};
    BankerResult result = {0};

    expect(scenario_create(&scenario, 4, 3, SCENARIO_SAFE), "safe scenario creates");
    expect(banker_analyze(&(BankerState){scenario.processes, scenario.resources, scenario.available, scenario.allocation, scenario.maximum}, &result), "safe scenario analyzes");
    expect(result.safe, "safe scenario is safe");
    expect(result.completed == 4, "safe scenario completes all processes");
    banker_result_destroy(&result);
    scenario_destroy(&scenario);
}

static void test_deadlock_scenario(void) {
    Scenario scenario = {0};
    BankerResult result = {0};

    expect(scenario_create(&scenario, 3, 3, SCENARIO_DEADLOCK), "deadlock scenario creates");
    expect(banker_analyze(&(BankerState){scenario.processes, scenario.resources, scenario.available, scenario.allocation, scenario.maximum}, &result), "deadlock scenario analyzes");
    expect(!result.safe, "deadlock scenario is unsafe");
    expect(result.completed == 0, "deadlock scenario has no completable process");
    banker_result_destroy(&result);
    scenario_destroy(&scenario);
}

static void test_request_statuses(void) {
    int available[] = {1};
    int allocation[] = {0, 1};
    int maximum[] = {1, 2};
    BankerState state = {2, 1, available, allocation, maximum};
    BankerResult result = {0};
    int grant[] = {1};
    int too_large[] = {2};
    int unavailable[] = {2};

    expect(banker_check_request(&state, 0, grant, &result) == BANKER_REQUEST_GRANTED, "safe request is granted");
    banker_result_destroy(&result);
    expect(banker_check_request(&state, 0, too_large, &result) == BANKER_REQUEST_EXCEEDS_NEED, "excessive request is rejected");
    expect(banker_check_request(&state, 1, unavailable, &result) == BANKER_REQUEST_EXCEEDS_NEED, "need is checked before availability");
    unavailable[0] = 2;
    maximum[1] = 3;
    expect(banker_check_request(&state, 1, unavailable, &result) == BANKER_REQUEST_UNAVAILABLE, "unavailable request waits");
}

static void test_invalid_state(void) {
    int available[] = {-1};
    int allocation[] = {0};
    int maximum[] = {1};
    BankerState state = {1, 1, available, allocation, maximum};

    expect(!banker_validate(&state), "negative available state is invalid");
}

int main(void) {
    test_safe_scenario();
    test_deadlock_scenario();
    test_request_statuses();
    test_invalid_state();
    if (failures != 0) {
        return 1;
    }
    puts("All Banker tests passed.");
    return 0;
}
