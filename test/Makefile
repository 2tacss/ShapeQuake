include ../config.mk

INCLUDES := -I. $(COMMON_INCLUDES)
BIN      := sq_test

SRCS     := $(wildcard *.c)
ALL_SRCS := $(SRCS) $(COMMON_SRCS)

all: $(BIN)

$(BIN): $(ALL_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

lsp:
	$(call gen_lsp, $(ALL_SRCS), compile_commands.json)

clean:
	rm -f $(BIN) compile_commands.json

.PHONY: all clean lsp
