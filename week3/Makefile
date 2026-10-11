CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11 -O2 -Iinclude -Isrc
LDFLAGS  := -lm
SRC_DIR  := src
TEST_DIR := test
OBJ_DIR  := build/obj
BIN_DIR  := build/bin

# Main simulator entry point (can be in src/scheduler.c or root scheduler.c)
MAIN_SRC  := $(SRC_DIR)/scheduler.c
MAIN_OBJ  := $(OBJ_DIR)/scheduler.o
TARGET    := $(BIN_DIR)/scheduler

# Discover all algorithm .c files in src/, EXCLUDING scheduler.c
ALGO_SRCS := $(filter-out $(MAIN_SRC), $(wildcard $(SRC_DIR)/*.c))

# Map src/<algo>.c -> build/obj/src_<algo>.o
ALGO_OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/src_%.o, $(ALGO_SRCS))

# Map src/<algo>.c -> build/bin/test_<algo>
TEST_BINS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/test_%, $(ALGO_SRCS))

.PHONY: all app test run clean

# Default target builds both the unit tests and the final simulator binary
all: test app

# Build the final unified simulator executable
app: $(TARGET)

# Run the unified simulator interactively
run: $(TARGET)
	./$(TARGET)

# Link scheduler.o + all algorithm object files (src_fcfs.o, src_sjf.o, etc.)
$(TARGET): $(MAIN_OBJ) $(ALGO_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile the main simulator entry point (keep its real main() intact!)
$(MAIN_OBJ): $(MAIN_SRC) include/schedule.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run all unit tests
test: $(TEST_BINS)
	@failed=0; \
	for bin in $(TEST_BINS); do \
		./$$bin || failed=1; \
	done; \
	if [ $$failed -ne 0 ]; then \
		echo "Suite status: TEST FAILED"; \
		exit 1; \
	else \
		echo "Suite status: ALL PASSED"; \
	fi

# Link individual unit test binaries
$(BIN_DIR)/test_%: $(OBJ_DIR)/test_%.o $(OBJ_DIR)/src_%.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile algorithm files: rename any intern's main() to main_<algo>() so they never collide
$(OBJ_DIR)/src_%.o: $(SRC_DIR)/%.c include/schedule.h | $(OBJ_DIR)
	@grep -q '#include *"schedule.h"' $< || (echo "[ERROR] $< must #include \"schedule.h\"" && exit 1)
	$(CC) $(CFLAGS) -Dmain=main_$* -c $< -o $@

# Compile test runners
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/test_%.c include/schedule.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf build bin
