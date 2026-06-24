# Makefile — mcugen
# Material Color Utilities Generator

CC      ?= gcc
CFLAGS  ?= -std=c11 -Os -Wall -Wextra -Wpedantic \
           -Wno-unused-parameter -Wno-unused-function \
           -Wno-format-truncation -Wno-stringop-truncation \
           -Iinclude -Isrc/lib
AR      ?= ar
ARFLAGS  = rcs

BIN     = build/mcugen
LIB     = build/libmcu.a
VERSION = 2.0.0

LIB_SRCS = \
    src/lib/utils/math_utils.c       \
    src/lib/utils/color_utils.c      \
    src/lib/utils/string_utils.c     \
    src/lib/hct/viewing_conditions.c \
    src/lib/hct/cam16.c              \
    src/lib/hct/hct_solver.c         \
    src/lib/hct/hct.c                \
    src/lib/blend/blend.c            \
    src/lib/contrast/contrast.c      \
    src/lib/dislike/dislike.c        \
    src/lib/palettes/tonal_palette.c \
    src/lib/score/score.c            \
    src/lib/temperature/temperature_cache.c

LIB_OBJS = $(patsubst src/%.c, build/%.o, $(LIB_SRCS))

all: $(BIN)

$(LIB): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BIN): src/main.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lmcu -lm -lpthread

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(BIN)
	install -Dm755 $(BIN) $(HOME)/.local/bin/mcugen
	@echo "Installed to $(HOME)/.local/bin/mcugen"
	@echo "Run 'mcugen init' to create starter config."

uninstall:
	rm -f $(HOME)/.local/bin/mcugen

clean:
	rm -rf build/

.PHONY: all install uninstall clean
