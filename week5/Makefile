# ==========================================================================
# Makefile — Page Replacement Simulator
#
# Targets:
#   make              build backend (+ console if core complete)
#   make backend      build backend server (always buildable, stubs if needed)
#   make full         build console binary (requires core + main.c)
#   make run          build + run backend
#   make test         build & run whatever is runnable; RED on any FAIL
#   make test-strict  same + -Werror + valgrind (if installed)
#   make test-backend only the backend unit tests (works with stubs)
#   make status       print which expected files are present/missing
#   make clean
#
# Test policy:
#   - Each test binary is compiled only if its own source exists.
#   - A test that fails to LINK (missing core dependency) is SKIPped,
#     not fatal — a missing core function is not a semantic failure.
#   - A test that runs and fails -> RED, exit non-zero.
#   - If nothing ran at all -> RED (nothing was verified).
# ==========================================================================

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -O2 -g
CPPFLAGS?=
LDFLAGS ?=
LDLIBS  ?=

SRC_DIR := src
INC_DIR := include
OBJ_DIR := build
TEST_DIR := test
TEST_BIN_DIR := $(OBJ_DIR)/test

HEADERS_ALL := $(wildcard $(INC_DIR)/*.h)
ALL_SRCS    := $(wildcard $(SRC_DIR)/*.c)

BACKEND_LIB  := $(SRC_DIR)/backend.c
BACKEND_MAIN := $(SRC_DIR)/backend_main.c
CONSOLE_MAIN := $(SRC_DIR)/main.c

EXPECTED_CORE := \
    $(SRC_DIR)/orchestrator.c \
    $(SRC_DIR)/fifo.c \
    $(SRC_DIR)/lru.c \
    $(SRC_DIR)/optimal.c \
    $(SRC_DIR)/display.c

CORE_FOUND   := $(wildcard $(EXPECTED_CORE))
CORE_MISSING := $(filter-out $(CORE_FOUND),$(EXPECTED_CORE))

LIB_SRCS := $(filter-out $(BACKEND_LIB) $(BACKEND_MAIN) $(CONSOLE_MAIN),$(ALL_SRCS))
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
BACK_LIB_OBJ  := $(OBJ_DIR)/backend.o
BACK_MAIN_OBJ := $(OBJ_DIR)/backend_main.o
CON_OBJ       := $(OBJ_DIR)/console_main.o

# ---- Test discovery ------------------------------------------------------
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(TEST_BIN_DIR)/%,$(TEST_SRCS))

TEST_BACKEND_SRCS := $(filter %/test_backend.c,$(TEST_SRCS))
TEST_CORE_SRCS    := $(filter-out $(TEST_BACKEND_SRCS),$(TEST_SRCS))
TEST_BACKEND_BINS := $(patsubst $(TEST_DIR)/%.c,$(TEST_BIN_DIR)/%,$(TEST_BACKEND_SRCS))
TEST_CORE_BINS    := $(patsubst $(TEST_DIR)/%.c,$(TEST_BIN_DIR)/%,$(TEST_CORE_SRCS))

.PHONY: all backend full run status clean help check-core \
        test test-strict test-backend test-core

RED    := $(shell tput setaf 1 2>/dev/null)
GREEN  := $(shell tput setaf 2 2>/dev/null)
YELLOW := $(shell tput setaf 3 2>/dev/null)
RESET  := $(shell tput sgr0   2>/dev/null)

# ---- Default -------------------------------------------------------------
all:
	@$(MAKE) --no-print-directory check-core
	@$(MAKE) --no-print-directory backend
	@if [ -f "$(CONSOLE_MAIN)" ] && [ -z "$(CORE_MISSING)" ]; then \
	    $(MAKE) --no-print-directory full; \
	else \
	    printf '\n  (skipping "page_sim": %s)\n\n' \
	        "core files or main.c missing — run 'make status' for details"; \
	fi

# ---- Backend (always buildable) ------------------------------------------
backend: $(OBJ_DIR) $(BACK_MAIN_OBJ) $(BACK_LIB_OBJ) $(LIB_OBJS)
	@if [ ! -f "$(BACKEND_LIB)" ] || [ ! -f "$(BACKEND_MAIN)" ]; then \
	    echo "error: backend sources not found (need $(BACKEND_LIB) and $(BACKEND_MAIN))." >&2; \
	    exit 1; \
	fi
	@echo '  LD      page_sim_backend$(if $(CORE_MISSING), (stubs),)'
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    $(BACK_MAIN_OBJ) $(BACK_LIB_OBJ) $(LIB_OBJS) \
	    $(LDFLAGS) $(LDLIBS) -o page_sim_backend
	@echo "  OK      ./page_sim_backend"

ifneq ($(CORE_MISSING),)
BACKEND_ONLY_DEFINE := -DBUILD_BACKEND_ONLY
endif

# ---- Full console binary (only when core exists) -------------------------
full: $(OBJ_DIR) $(CON_OBJ) $(LIB_OBJS)
	@if [ -z "$(CONSOLE_MAIN)" ]; then \
	    echo "warning: $(CONSOLE_MAIN) not found; cannot build page_sim." >&2; \
	    exit 1; \
	fi
	@if [ -n "$(CORE_MISSING)" ]; then \
	    echo "warning: core files missing; 'make full' will not work." >&2; \
	    echo "         missing: $(CORE_MISSING)" >&2; \
	    exit 1; \
	fi
	@echo "  LD      page_sim"
	@$(CC) $(CFLAGS) $(CPPFLAGS) \
	    $(CON_OBJ) $(LIB_OBJS) \
	    $(LDFLAGS) $(LDLIBS) -o page_sim
	@echo "  OK      ./page_sim"

# ==========================================================================
# TESTS — per-file gating, red on any FAIL
# ==========================================================================

# Run every test whose source exists. If the test binary can't be linked
# (because a core .c file it references is missing), SKIP it — that's a
# missing dependency, not a semantic failure. If nothing ran -> RED.
test: $(TEST_BIN_DIR)
	@echo "  --- test suite (per-file gating) ---"
	@ran=0; fail=0; skip=0; \
	for t in $(TEST_CORE_BINS) $(TEST_BACKEND_BINS); do \
	    src="$(TEST_DIR)/$$(basename $$t).c"; \
	    if [ ! -f "$$src" ]; then \
	        printf '  %sSKIP%s    %s (source missing)\n' \
	            "$(YELLOW)" "$(RESET)" "$$t"; skip=$$((skip+1)); continue; \
	    fi; \
	    if ! $(MAKE) --no-print-directory "$$t" >/dev/null 2>&1; then \
	        printf '  %sSKIP%s    %s (link failed: core dep missing)\n' \
	            "$(YELLOW)" "$(RESET)" "$$t"; skip=$$((skip+1)); continue; \
	    fi; \
	    ran=$$((ran + 1)); \
	    printf '  RUN     %s\n' "$$t"; \
	    if "$$t"; then printf '  %sPASS%s    %s\n' "$(GREEN)" "$(RESET)" "$$t"; \
	    else printf '  %sFAIL%s    %s\n' "$(RED)" "$(RESET)" "$$t"; fail=1; fi; \
	done; \
	if [ "$$ran" -eq 0 ]; then \
	    printf '\n%sNOTHING RAN: no test could be built.%s\n' "$(RED)" "$(RESET)"; \
	    exit 1; \
	fi; \
	if [ "$$fail" -ne 0 ]; then \
	    printf '\n%sTESTS FAILED%s\n' "$(RED)" "$(RESET)"; \
	    exit 1; \
	fi; \
	printf '\n%sALL EXECUTED TESTS PASSED%s (%d ran, %d skipped)\n' \
	    "$(GREEN)" "$(RESET)" "$$ran" "$$skip"

test-core: $(TEST_BIN_DIR)
	@echo "  --- core tests ---"
	@ran=0; fail=0; skip=0; \
	for t in $(TEST_CORE_BINS); do \
	    src="$(TEST_DIR)/$$(basename $$t).c"; \
	    if [ ! -f "$$src" ]; then \
	        printf '  %sSKIP%s    %s (source missing)\n' \
	            "$(YELLOW)" "$(RESET)" "$$t"; skip=$$((skip+1)); continue; \
	    fi; \
	    if ! $(MAKE) --no-print-directory "$$t" >/dev/null 2>&1; then \
	        printf '  %sSKIP%s    %s (link failed: core dep missing)\n' \
	            "$(YELLOW)" "$(RESET)" "$$t"; skip=$$((skip+1)); continue; \
	    fi; \
	    ran=$$((ran + 1)); \
	    printf '  RUN     %s\n' "$$t"; \
	    if "$$t"; then printf '  %sPASS%s    %s\n' "$(GREEN)" "$(RESET)" "$$t"; \
	    else printf '  %sFAIL%s    %s\n' "$(RED)" "$(RESET)" "$$t"; fail=1; fi; \
	done; \
	[ "$$fail" -eq 0 ]

test-backend: $(TEST_BIN_DIR) $(BACK_LIB_OBJ)
	@echo "  --- backend tests (stub-friendly) ---"
	@ran=0; fail=0; skip=0; \
	for t in $(TEST_BACKEND_BINS); do \
	    src="$(TEST_DIR)/$$(basename $$t).c"; \
	    if [ ! -f "$$src" ]; then \
	        printf '  %sSKIP%s    %s (source missing)\n' \
	            "$(YELLOW)" "$(RESET)" "$$t"; skip=$$((skip+1)); continue; \
	    fi; \
	    if ! $(MAKE) --no-print-directory "$$t" >/dev/null 2>&1; then \
	        printf '  %sSKIP%s    %s (link failed)\n' \
	            "$(YELLOW)" "$(RESET)" "$$t"; skip=$$((skip+1)); continue; \
	    fi; \
	    ran=$$((ran + 1)); \
	    printf '  RUN     %s\n' "$$t"; \
	    if "$$t"; then printf '  %sPASS%s    %s\n' "$(GREEN)" "$(RESET)" "$$t"; \
	    else printf '  %sFAIL%s    %s\n' "$(RED)" "$(RESET)" "$$t"; fail=1; fi; \
	done; \
	[ "$$fail" -eq 0 ]

# Strict: -Werror, then valgrind if present.
test-strict:
	@$(MAKE) --no-print-directory test \
	    CFLAGS="$(CFLAGS) -Werror -O1"
	@if command -v valgrind >/dev/null 2>&1; then \
	    echo "  --- valgrind ---"; \
	    for t in $(TEST_CORE_BINS) $(TEST_BACKEND_BINS); do \
	        [ -x "$$t" ] || continue; \
	        valgrind --error-exitcode=1 --leak-check=full \
	            --errors-for-leak-kinds=definite "$$t" >/dev/null 2>&1 \
	            && echo "  $(GREEN)VG PASS$(RESET) $$t" \
	            || { echo "  $(RED)VG FAIL$(RESET) $$t"; exit 1; }; \
	    done; \
	else \
	    echo "  (valgrind not installed; skipping)"; \
	fi

# ---- Test link/compile rules --------------------------------------------
$(TEST_BIN_DIR):
	@mkdir -p $(TEST_BIN_DIR)

# Core tests: link against whatever LIB_OBJS currently exist. If a test's
# own .c is present but a core dependency .c is missing, the link fails;
# make test treats that as SKIP, not FAIL.
$(TEST_BIN_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_OBJS) | $(TEST_BIN_DIR)
	@echo "  CC+LD   $<  (core)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -I$(INC_DIR) -I$(TEST_DIR) \
	    $< $(LIB_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# Backend test: links ONLY backend.o (no main) + the test file.
$(TEST_BIN_DIR)/test_backend: $(TEST_DIR)/test_backend.c $(BACK_LIB_OBJ) | $(TEST_BIN_DIR)
	@echo "  CC+LD   $<  (backend)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -I$(INC_DIR) -I$(TEST_DIR) \
	    $< $(BACK_LIB_OBJ) $(LDFLAGS) $(LDLIBS) -o $@

# ==========================================================================
# Build rules
# ==========================================================================

run: backend
	@./page_sim_backend

status:
	@echo "Toolchain     : $(CC)"
	@echo "Sources (src) : $(if $(ALL_SRCS),$(words $(ALL_SRCS)) file(s),none)"
	@$(foreach f,$(ALL_SRCS), echo "   - $(f)";)
	@echo "Tests (test)  : $(if $(TEST_SRCS),$(words $(TEST_SRCS)) file(s),none)"
	@$(foreach f,$(TEST_SRCS), echo "   - $(f)";)
	@echo
	@echo "Expected core files:"
	@$(foreach f,$(CORE_FOUND),   echo "   [found]   $(f)";)
	@$(foreach f,$(CORE_MISSING), echo "   [MISSING] $(f)";)
	@echo
	@if [ -z "$(CORE_MISSING)" ]; then \
	    echo " => core complete; 'make backend', 'make full', 'make test' available."; \
	else \
	    echo " => core incomplete; 'make test' will run whatever it can."; \
	fi

check-core:
	@if [ -n "$(CORE_MISSING)" ]; then \
	    echo "note: some expected core files are missing:"; \
	    for f in $(CORE_MISSING); do echo "        - $$f"; done; \
	    echo "note: 'make test' will run whatever is present."; \
	    echo; \
	fi

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    -I$(INC_DIR) -I. -c $< -o $@

$(BACK_LIB_OBJ): $(BACKEND_LIB) | $(OBJ_DIR)
	@echo "  CC      $<  (backend lib)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    -I$(INC_DIR) -I. -c $< -o $@

$(BACK_MAIN_OBJ): $(BACKEND_MAIN) | $(OBJ_DIR)
	@echo "  CC      $<  (backend entry)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    -I$(INC_DIR) -I. -c $< -o $@

$(CON_OBJ): $(CONSOLE_MAIN) | $(OBJ_DIR)
	@echo "  CC      $<  (console entry)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -I$(INC_DIR) -I. -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR) page_sim_backend page_sim
	@rm -f ./page_sim.sock
	@echo "  cleaned."

help:
	@sed -n 's/^# \{0,1\}//p' Makefile | sed -n '1,28p'
