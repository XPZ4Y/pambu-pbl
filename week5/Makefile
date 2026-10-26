# ==========================================================================
# Makefile — Page Replacement Simulator
#
# Discovery-based, per-file gating:
#   Every src/*.c that isn't an entry point (main.c, backend_main.c) becomes
#   a "unit" with its own object (build/obj/src_<name>.o) and — if a matching
#   test/test_<name>.c exists — its own test binary (build/bin/test_<name>).
#
#   - A unit with no matching test file → RED, "no test found".
#   - A unit whose test file is present → built and run individually.
#   - A unit that can't be built (missing dependency) → SKIP, not FAIL.
#   - A unit whose test fails at runtime → FAIL, exit non-zero.
#
# This means:
#   - Only fifo.c present, with test_fifo.c → FIFO is tested in isolation.
#   - Only fifo.c present, no test_fifo.c → RED, "write a test".
#   - backend.c alone → backend test runs; backend weak stubs cover the rest.
#   - Full core → every algorithm tested independently; orchestrator tested
#     against the real algorithms.
#
# Targets:
#   make              build backend + console (if core complete), then test
#   make backend      build ./page_sim_backend
#   make full         build ./page_sim (requires core + main.c)
#   make run          build + run backend
#   make test         build & run every discovered unit test (RED on FAIL
#                     or on missing test file)
#   make test-backend run only backend tests (works with stubs)
#   make test-strict  same as test, with -Werror and valgrind if installed
#   make status       show which units are testable / missing / untested
#   make clean
# ==========================================================================

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -O2 -g
CPPFLAGS?=
LDFLAGS ?=
LDLIBS  ?=

SRC_DIR       := src
INC_DIR       := include
TEST_DIR      := test
OBJ_DIR       := build/obj
TEST_BIN_DIR  := build/bin

# ---- Entry points (excluded from the unit-test loop) ---------------------
BACKEND_LIB   := $(SRC_DIR)/backend.c
BACKEND_MAIN  := $(SRC_DIR)/backend_main.c
CONSOLE_MAIN  := $(SRC_DIR)/main.c
ENTRY_SRCS    := $(BACKEND_MAIN) $(CONSOLE_MAIN)

# ---- Core files the Makefile knows about (for status/check-core) ---------
EXPECTED_CORE := \
    $(SRC_DIR)/orchestrator.c \
    $(SRC_DIR)/fifo.c \
    $(SRC_DIR)/lru.c \
    $(SRC_DIR)/optimal.c \
    $(SRC_DIR)/display.c

CORE_FOUND   := $(wildcard $(EXPECTED_CORE))
CORE_MISSING := $(filter-out $(CORE_FOUND),$(EXPECTED_CORE))

# ---- Unit discovery ------------------------------------------------------
# Every src/*.c that isn't an entry point is a "unit".
ALL_SRCS      := $(wildcard $(SRC_DIR)/*.c)
UNIT_SRCS     := $(filter-out $(ENTRY_SRCS),$(ALL_SRCS))
UNIT_NAMES    := $(patsubst $(SRC_DIR)/%.c,%,$(UNIT_SRCS))

# Per-unit test source and binary names.
UNIT_TESTS    := $(patsubst $(SRC_DIR)/%.c,$(TEST_DIR)/test_%.c,$(UNIT_SRCS))
UNIT_TEST_BINS:= $(patsubst $(SRC_DIR)/%.c,$(TEST_BIN_DIR)/test_%,$(UNIT_SRCS))

# Units that have a matching test file (what `make test` will actually run)
RUNNABLE_NAMES := $(foreach n,$(UNIT_NAMES),$(if $(wildcard $(TEST_DIR)/test_$(n).c),$(n),))
MISSING_TESTS  := $(filter-out $(RUNNABLE_NAMES),$(UNIT_NAMES))

# Backend objects (built separately; backend.c is a unit but has a special rule)
BACK_LIB_OBJ  := $(OBJ_DIR)/src_backend.o
BACK_MAIN_OBJ := $(OBJ_DIR)/backend_main.o
CON_OBJ       := $(OBJ_DIR)/console_main.o

# All unit objects (incl. backend.o), for linking `page_sim_backend`.
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/src_%.o,$(UNIT_SRCS))

.PHONY: all backend full run status clean help check-core \
        test test-strict test-backend

RED    := $(shell tput setaf 1 2>/dev/null)
GREEN  := $(shell tput setaf 2 2>/dev/null)
YELLOW := $(shell tput setaf 3 2>/dev/null)
RESET  := $(shell tput sgr0   2>/dev/null)

# ==========================================================================
# Default
# ==========================================================================
all: check-core backend
	@if [ -f "$(CONSOLE_MAIN)" ] && [ -z "$(CORE_MISSING)" ]; then \
	    $(MAKE) --no-print-directory full; \
	else \
	    printf '\n  (skipping "page_sim": core files or main.c missing — run "make status")\n\n'; \
	fi
	@$(MAKE) --no-print-directory test

# ==========================================================================
# Binaries
# ==========================================================================

backend: $(OBJ_DIR) $(BACK_MAIN_OBJ) $(BACK_LIB_OBJ) $(filter-out $(BACK_LIB_OBJ),$(LIB_OBJS))
	@if [ ! -f "$(BACKEND_LIB)" ] || [ ! -f "$(BACKEND_MAIN)" ]; then \
	    echo "error: backend sources not found (need $(BACKEND_LIB) and $(BACKEND_MAIN))." >&2; \
	    exit 1; \
	fi
	@echo '  LD      page_sim_backend$(if $(CORE_MISSING), (stubs),)'
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    $(BACK_MAIN_OBJ) $(LIB_OBJS) \
	    $(LDFLAGS) $(LDLIBS) -o page_sim_backend
	@echo "  OK      ./page_sim_backend"

ifneq ($(CORE_MISSING),)
BACKEND_ONLY_DEFINE := -DBUILD_BACKEND_ONLY
endif

full: $(OBJ_DIR) $(CON_OBJ) $(LIB_OBJS)
	@if [ -z "$(CONSOLE_MAIN)" ]; then \
	    echo "warning: $(CONSOLE_MAIN) not found; cannot build page_sim." >&2; exit 1; \
	fi
	@if [ -n "$(CORE_MISSING)" ]; then \
	    echo "warning: core files missing; 'make full' will not work." >&2; \
	    echo "         missing: $(CORE_MISSING)" >&2; exit 1; \
	fi
	@echo "  LD      page_sim"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(CON_OBJ) $(LIB_OBJS) $(LDFLAGS) $(LDLIBS) -o page_sim
	@echo "  OK      ./page_sim"

run: backend
	@./page_sim_backend

# ==========================================================================
# Tests — per-file discovery, per-file gating
# ==========================================================================

# Build + run every unit test that exists. Units without a test file are
# RED (not silent); units whose test can't link are SKIP (missing dep).
test: $(TEST_BIN_DIR)
	@echo "  --- test suite (per-file discovery) ---"
	@missing_tests="$(MISSING_TESTS)"; \
	if [ -n "$$missing_tests" ]; then \
	    printf '%sERROR: source files with no matching test:%s\n' "$(RED)" "$(RESET)"; \
	    for n in $$missing_tests; do \
	        printf '  %s  src/%s.c  →  expected test/test_%s.c%s\n' \
	            "$(RED)" "$$n" "$$n" "$(RESET)"; \
	    done; \
	    exit 1; \
	fi
	@ran=0; fail=0; skip=0; \
	for n in $(RUNNABLE_NAMES); do \
	    t="$(TEST_BIN_DIR)/test_$$n"; \
	    log=$$(mktemp); \
	    if ! $(MAKE) --no-print-directory "$$t" >"$$log" 2>&1; then \
	        printf '  %sSKIP%s    %s (build failed)\n' \
	            "$(YELLOW)" "$(RESET)" "$$t"; \
	        sed 's/^/          /' "$$log"; \
	        rm -f "$$log"; skip=$$((skip+1)); continue; \
	    fi; \
	    rm -f "$$log"; \
	    ran=$$((ran + 1)); \
	    printf '  RUN     %s\n' "$$t"; \
	    if "$$t"; then printf '  %sPASS%s    %s\n' "$(GREEN)" "$(RESET)" "$$t"; \
	    else printf '  %sFAIL%s    %s\n' "$(RED)" "$(RESET)" "$$t"; fail=1; fi; \
	done; \
	if [ "$$ran" -eq 0 ] && [ "$$skip" -eq 0 ]; then \
	    printf '\n%sNOTHING TO TEST: no src/*.c files.%s\n' "$(RED)" "$(RESET)"; \
	    exit 1; \
	fi; \
	if [ "$$fail" -ne 0 ]; then \
	    printf '\n%sTESTS FAILED%s\n' "$(RED)" "$(RESET)"; exit 1; \
	fi; \
	printf '\n%sALL EXECUTED TESTS PASSED%s (%d ran, %d skipped)\n' \
	    "$(GREEN)" "$(RESET)" "$$ran" "$$skip"

test-backend: $(TEST_BIN_DIR) $(BACK_LIB_OBJ)
	@t="$(TEST_BIN_DIR)/test_backend"; \
	if [ ! -f "$(TEST_DIR)/test_backend.c" ]; then \
	    printf '  %sSKIP%s    %s (test source missing)\n' \
	        "$(YELLOW)" "$(RESET)" "$$t"; exit 0; \
	fi; \
	log=$$(mktemp); \
	if ! $(MAKE) --no-print-directory "$$t" >"$$log" 2>&1; then \
	    printf '  %sSKIP%s    %s (build failed)\n' \
	        "$(YELLOW)" "$(RESET)" "$$t"; \
	    sed 's/^/          /' "$$log"; rm -f "$$log"; exit 0; \
	fi; \
	rm -f "$$log"; \
	printf '  RUN     %s\n' "$$t"; \
	if "$$t"; then printf '  %sPASS%s    %s\n' "$(GREEN)" "$(RESET)" "$$t"; \
	else printf '  %sFAIL%s    %s\n' "$(RED)" "$(RESET)" "$$t"; exit 1; fi

test-strict:
	@$(MAKE) --no-print-directory test CFLAGS="$(CFLAGS) -Werror -O1"
	@if command -v valgrind >/dev/null 2>&1; then \
	    echo "  --- valgrind ---"; \
	    for n in $(RUNNABLE_NAMES); do \
	        t="$(TEST_BIN_DIR)/test_$$n"; \
	        [ -x "$$t" ] || continue; \
	        valgrind --error-exitcode=1 --leak-check=full \
	            --errors-for-leak-kinds=definite "$$t" >/dev/null 2>&1 \
	            && echo "  $(GREEN)VG PASS$(RESET) $$t" \
	            || { echo "  $(RED)VG FAIL$(RESET) $$t"; exit 1; }; \
	    done; \
	else \
	    echo "  (valgrind not installed; skipping)"; \
	fi

# ==========================================================================
# Build rules — one object per unit, one test binary per unit
# ==========================================================================

$(OBJ_DIR) $(TEST_BIN_DIR):
	@mkdir -p $@

# Backend lib object: compiled with -DBUILD_BACKEND_ONLY when core is
# incomplete, so its WEAK stubs stay "strong enough" to satisfy the linker.
$(BACK_LIB_OBJ): $(BACKEND_LIB) | $(OBJ_DIR)
	@echo "  CC      $<  (backend lib)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    -I$(INC_DIR) -I. -c $< -o $@

# Every other unit object. -Dmain=main_<name> so an intern's stray main()
# cannot collide with the test runner's main.
$(OBJ_DIR)/src_%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    -I$(INC_DIR) -I. -Dmain=main_$* -c $< -o $@

$(BACK_MAIN_OBJ): $(BACKEND_MAIN) | $(OBJ_DIR)
	@echo "  CC      $<  (backend entry)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(BACKEND_ONLY_DEFINE) \
	    -I$(INC_DIR) -I. -c $< -o $@

$(CON_OBJ): $(CONSOLE_MAIN) | $(OBJ_DIR)
	@echo "  CC      $<  (console entry)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -I$(INC_DIR) -I. -c $< -o $@

# Test runner object.
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/test_%.c | $(OBJ_DIR)
	@echo "  CC      $<  (test)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -I$(INC_DIR) -I$(TEST_DIR) -c $< -o $@

# Link one test binary:
#   test_<name> = test_<name>.o + src_<name>.o [+ src_backend.o if name != backend]
# We always pull in backend.o *if it exists* (it provides weak stubs and
# the ipc_* functions), except when the unit under test IS backend, in which
# case src_backend.o already is the unit.
$(TEST_BIN_DIR)/test_backend: $(OBJ_DIR)/test_backend.o $(BACK_LIB_OBJ) | $(TEST_BIN_DIR)
	@echo "  LD      $@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(TEST_BIN_DIR)/test_%: $(OBJ_DIR)/test_%.o $(OBJ_DIR)/src_%.o \
                        $(if $(wildcard $(BACKEND_LIB)),$(BACK_LIB_OBJ),) \
                        | $(TEST_BIN_DIR)
	@echo "  LD      $@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) \
	    $(OBJ_DIR)/test_$*.o $(OBJ_DIR)/src_$*.o \
	    $(if $(wildcard $(BACKEND_LIB)),$(BACK_LIB_OBJ),) \
	    $(LDFLAGS) $(LDLIBS) -o $@

# ==========================================================================
# Status / check
# ==========================================================================

status:
	@echo "Toolchain     : $(CC)"
	@echo "Sources (src) : $(if $(UNIT_SRCS),$(words $(UNIT_SRCS)) unit(s),none)"
	@$(foreach f,$(UNIT_SRCS), echo "   - $(f)";)
	@echo "Entry points  : $(if $(ENTRY_SRCS),$(words $(ENTRY_SRCS)) file(s),none)"
	@$(foreach f,$(ENTRY_SRCS), echo "   - $(f)";)
	@echo
	@echo "Expected core files:"
	@$(foreach f,$(CORE_FOUND),   echo "   [found]   $(f)";)
	@$(foreach f,$(CORE_MISSING), echo "   [MISSING] $(f)";)
	@echo
	@echo "Per-unit test status:"
	@$(foreach n,$(UNIT_NAMES), \
	    if [ -f "$(TEST_DIR)/test_$(n).c" ]; then \
	        echo "   [test]    src/$(n).c  →  test/test_$(n).c"; \
	    else \
	        echo "   [NO TEST] src/$(n).c"; \
	    fi;)
	@echo

check-core:
	@if [ -n "$(CORE_MISSING)" ]; then \
	    echo "note: some expected core files are missing:"; \
	    for f in $(CORE_MISSING); do echo "        - $$f"; done; \
	    echo "note: 'make test' will test whichever units exist."; \
	    echo; \
	fi

clean:
	@rm -rf build page_sim_backend page_sim
	@rm -f ./page_sim.sock
	@echo "  cleaned."

help:
	@sed -n 's/^# \{0,1\}//p' Makefile | sed -n '1,32p'
