include ../config.mk

INCLUDES := -Iinclude $(COMMON_INCLUDES) -I../shell/include
LDLIBS   := -lpthread -lsqlite3
BIN      := sq_server

SRCS     := $(shell find src -name "*.c")
ALL_SRCS := $(SRCS) $(COMMON_SRCS)

all: $(BIN)

$(BIN): $(ALL_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDLIBS)

lsp:
	$(call gen_lsp, $(ALL_SRCS), compile_commands.json)

clean:
	rm -f $(BIN) compile_commands.json

.PHONY: all clean lsp
