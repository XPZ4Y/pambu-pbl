# Test coverage

`make test` configures the CMake build, compiles the CLI and test executable, then runs CTest.

The test executable checks these cases:

1. A generated safe scenario completes every process.
2. A circular-wait scenario is unsafe and has no initial completable process.
3. A safe request is granted.
4. A request beyond a declared maximum need is rejected.
5. A request that cannot be satisfied by available resources waits.
6. A state with negative availability is rejected as invalid.

The test suite is intentionally dependency-free and uses only the C standard library and CTest.
