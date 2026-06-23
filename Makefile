# Makefile — mcugen
# Material Color Utilities Generator
# Automatically ported by Claude (Anthropic). MIT License.

CC      ?= gcc
CFLAGS  ?= -std=c11 -Os -Wall -Wextra -Wpedantic -Wno-unused-parameter \
           -Wno-unused-function -Wno-format-truncation -Wno-stringop-truncation -fPIC -I.
AR      ?= ar
ARFLAGS  = rcs

BIN  = mcugen
VERSION = 2.0.0
LIB  = libmcu.a

SRCS = \
    utils/math_utils.c       \
    utils/color_utils.c      \
    utils/string_utils.c     \
    hct/viewing_conditions.c \
    hct/cam16.c              \
    hct/hct_solver.c         \
    hct/hct.c                \
    blend/blend.c            \
    contrast/contrast.c      \
    dislike/dislike.c        \
    palettes/tonal_palette.c \
    score/score.c            \
    temperature/temperature_cache.c

OBJS = $(SRCS:.c=.o)

all: $(BIN)

# Static library from MCU sources
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Single executable: mcugen.c + libmcu.a
$(BIN): mcugen.c $(LIB)
	$(CC) $(CFLAGS) -o $@ mcugen.c -L. -lmcu -lm -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Install to ~/.local/bin
install: $(BIN)
	install -Dm755 $(BIN) $(HOME)/.local/bin/$(BIN)
	@echo "Installed to $(HOME)/.local/bin/$(BIN)"
	@echo "Run 'mcugen init' to create starter config."

uninstall:
	rm -f $(HOME)/.local/bin/$(BIN)

clean:
	rm -f $(OBJS) $(LIB) $(BIN)

.PHONY: all install uninstall clean
