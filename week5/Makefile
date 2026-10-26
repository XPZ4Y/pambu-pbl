CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11 -Iinclude
SRCDIR  := src
BUILDDIR:= build
TARGET  := $(BUILDDIR)/simulator

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c include/orchestrator.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
