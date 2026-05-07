include ../config.mk

INCLUDES      := -Iinclude $(COMMON_INCLUDES) -I../terminal/include/pty
LDFLAGS_PATHS := -L../terminal
LDLIBS        := -lsq_terminal -lutil -lpthread
BIN           := sq_shell

SRCS          := $(shell find src -name "*.c")
ALL_SRCS      := $(SRCS) $(COMMON_SRCS)

all: $(BIN)

$(BIN): $(ALL_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS_PATHS) $(LDLIBS)

test: $(ALL_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -DDEBUG $^ -o $(BIN)_test $(LDFLAGS_PATHS) $(LDLIBS)

deploy: $(ALL_SRCS)
	$(CC) -std=c23 -Wall -Wextra -O2 $(INCLUDES) -DDEPLOY $^ -o $(BIN)_deploy $(LDFLAGS_PATHS) $(LDLIBS)

lsp:
	$(call gen_lsp, $(ALL_SRCS), compile_commands.json)

clean:
	rm -f $(BIN) $(BIN)_test $(BIN)_deploy compile_commands.json

.PHONY: all clean lsp test deploy
