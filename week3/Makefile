
CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11 -O2 -Iinclude -Isrc -I.
SRC_DIR  := src
TEST_DIR := test
OBJ_DIR  := build/obj
BIN_DIR  := build/bin

SRCS      := $(wildcard $(SRC_DIR)/*.c)
TEST_BINS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/test_%, $(SRCS))

.PHONY: all test clean

all: test

# Run all test binaries and report overall status
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

$(BIN_DIR)/test_%: $(OBJ_DIR)/test_%.o $(OBJ_DIR)/src_%.o | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/src_%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -Dmain=tested_main -c $< -o $@

$(OBJ_DIR)/test_%.o: $(TEST_DIR)/test_%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf build
