# Week 4 Deadlock Lab

This folder preserves the three original browser lessons under `frontend/` and provides a lightweight C implementation of the interactive Banker's Algorithm behavior.

## Build and test

```sh
cd week4
make build
make test
```

The build creates `build/deadlock_lab`. Launch the desktop GUI with:

```sh
python3 gui.py
```

The GUI provides editable process and resource counts, a scenario selector, and displays the safety result from the C simulator. Its optional first argument is an alternate path to the `deadlock_lab` executable.

Run a deterministic scenario with:

```sh
./build/deadlock_lab safe 4 3
./build/deadlock_lab deadlock 3 3
./build/deadlock_lab mixed 5 3
```

Arguments are `scenario`, `processes`, and `resources`. Scenarios are `safe`, `deadlock`, and `mixed`. Processes are limited to 1 through 8 and resource types to 1 through 5, matching the original simulation controls.

## Modules

- `src/banker.c` validates a state, finds a safe sequence, and evaluates resource requests.
- `src/scenario.c` generates repeatable safe, deadlock, and mixed demonstrations.
- `src/main.c` is the small command-line presentation layer.
- `test/test_banker.c` covers safe and unsafe states, requests, and invalid input.
- `frontend/` contains the relocated HTML demonstrations.

## Behavior

A safe state has a sequence where each process can satisfy its remaining need from the current available resources and allocations released by earlier completed processes. A request is granted only when it is within the selected process need, immediately available, and leaves a safe state.
