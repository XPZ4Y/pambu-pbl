# Makefile for PAMBU CPU project
# Builds 3 binaries in the build/ folder: core, ui, logger

# --- Variables ---
CC      := gcc
CFLAGS  := -Wall -Wextra -O2
LDLIBS  := -lrt
NCLIBS  := -lncurses

# Build folder
BUILD   := build

# Source files
CORE_SRC   := src/core.c
UI_SRC     := src/pambuui.c
LOGGER_SRC := src/logger.c

# Target binaries
CORE_BIN   := $(BUILD)/core
UI_BIN     := $(BUILD)/ui
LOGGER_BIN := $(BUILD)/logger

# Default target
.PHONY: all
all: $(CORE_BIN) $(UI_BIN) $(LOGGER_BIN)

# --- Make build folder ---
$(BUILD):
	mkdir -p $(BUILD)

# --- Build core ---
$(CORE_BIN): $(CORE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# --- Build ui (matches the working command line) ---
$(UI_BIN): $(UI_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(NCLIBS) $(LDLIBS)

# --- Build logger ---
$(LOGGER_BIN): $(LOGGER_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# --- Remove build folder ---
.PHONY: clean
clean:
	rm -rf $(BUILD)
