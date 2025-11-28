# Detect platform (Linux vs macOS/Darwin)
UNAME_S := $(shell uname -s)

# Compilers (override from env if desired)
CC      ?= cc
AFL_CC  ?= afl-clang-fast

# Directories
SRC_DIR       := src
TEST_SRC_DIR  := test/src
TEST_BIN_DIR  := test/bin
TEST_TMP_DIR  := test/tmp

# Core sources (library-ish)
LIB_SRCS := $(SRC_DIR)/parse-http-date.c \
            $(SRC_DIR)/util-ahocorasick.c
LIB_OBJS := $(LIB_SRCS:.c=.o)

# Test sources: adjust if your filenames differ
TEST_SRCS := $(TEST_SRC_DIR)/date-main.c \
             $(TEST_SRC_DIR)/date-apache.c \
             $(TEST_SRC_DIR)/date-nginx.c \
             $(TEST_SRC_DIR)/date-litespeed.c \
             $(TEST_SRC_DIR)/date-lightd.c \
             $(TEST_SRC_DIR)/date-simple.c
TEST_OBJS := $(TEST_SRCS:.c=.o)

# Binaries
DATE_BIN         := $(TEST_BIN_DIR)/date
DATE_BIN_VALGRIND:= $(TEST_BIN_DIR)/date-valgrind
DATE_BIN_AFL     := $(TEST_BIN_DIR)/date-afl

# Base CFLAGS: warnings, debug info, optimization, frame pointer
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g -O2 -fno-omit-frame-pointer

# Platform-specific hardening
CFLAGS_PLATFORM  :=
LDFLAGS_PLATFORM :=

ifeq ($(UNAME_S),Linux)
    # glibc-style hardening
    CFLAGS_PLATFORM  += -D_FORTIFY_SOURCE=2 -fstack-protector-strong \
                        -fstack-clash-protection -fPIE
    LDFLAGS_PLATFORM += -Wl,-z,relro,-z,now -pie
endif

ifeq ($(UNAME_S),Darwin)
    # PIE mostly default; no FORTIFY_SOURCE here
    CFLAGS_PLATFORM  += -fstack-protector-strong
endif

CFLAGS  += $(CFLAGS_PLATFORM)
LDFLAGS += $(LDFLAGS_PLATFORM)

# Special configs
VALGRIND_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g3 -O0 -fno-omit-frame-pointer
AFL_CFLAGS      := -std=c11 -Wall -Wextra -g -O1 -fno-omit-frame-pointer

.PHONY: all clean valgrind afl test

all: $(DATE_BIN)

$(TEST_BIN_DIR):
	@mkdir -p $(TEST_BIN_DIR)

$(TEST_TMP_DIR):
	@mkdir -p $(TEST_TMP_DIR)

# Generic .c -> .o
%.o: %.c Makefile
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Normal hardened build
$(DATE_BIN): $(LIB_OBJS) $(TEST_OBJS) | $(TEST_BIN_DIR)
	@echo ""
	@echo "Linking $(DATE_BIN)"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Valgrind build: compile & link in one shot with valgrind-friendly flags
$(DATE_BIN_VALGRIND): | $(TEST_BIN_DIR)
	@echo "Linking $(DATE_BIN_VALGRIND)"
	@$(CC) $(VALGRIND_CFLAGS) $(CFLAGS_PLATFORM) \
	      $(TEST_SRCS) $(LIB_SRCS) \
	      -o $@ $(LDFLAGS_PLATFORM)

# AFL build: compile & link in one shot with AFL instrumentation
$(DATE_BIN_AFL): | $(TEST_BIN_DIR)
	@echo "Linking $(DATE_BIN_AFL)"
	@$(AFL_CC) $(AFL_CFLAGS) $(CFLAGS_PLATFORM) \
	          $(TEST_SRCS) $(LIB_SRCS) \
	          -o $@ $(LDFLAGS_PLATFORM)

# Convenience targets
valgrind: $(DATE_BIN_VALGRIND)
afl:      $(DATE_BIN_AFL)

GOOD_EXPECTED := test/data/date-good.txt
BAD_EXPECTED  := test/data/date-bad.txt
GOOD_OUT      := $(TEST_TMP_DIR)/date-good.out
BAD_OUT       := $(TEST_TMP_DIR)/date-bad.out

test: $(TEST_BIN_DIR)
	@echo "date: [1/3] --good"
	@$(DATE_BIN) --good > $(GOOD_OUT)
	@if ! diff -u $(GOOD_EXPECTED) $(GOOD_OUT); then \
	    echo "FAILED: Output for --good does not match expected."; \
	    exit 1; \
	fi
	
	@echo "date: [2/3] --bad"
	@$(DATE_BIN) --bad > $(BAD_OUT)
	@if ! diff -u $(BAD_EXPECTED) $(BAD_OUT); then \
	    echo "FAILED: Output for --bad does not match expected."; \
	    exit 1; \
	fi

	@echo "date: [3/3] Apache"
	@$(DATE_BIN) --file test/data/date-apache.txt > test/tmp/date-apache.out
	@if ! diff -u test/data/date-apache.txt test/tmp/date-apache.out; then \
	    echo "FAILED: Output for Apache does not match expected."; \
	    exit 1; \
	fi

	@echo "All tests passed."

clean:
	rm -f $(LIB_OBJS) $(TEST_OBJS) \
	      $(DATE_BIN) $(DATE_BIN_VALGRIND) $(DATE_BIN_AFL)

