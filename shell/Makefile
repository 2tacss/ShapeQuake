include ../config.mk

INCLUDES      := -Iinclude $(COMMON_INCLUDES) -I../terminal/include/pty
LDFLAGS_PATHS := -L../terminal
LDLIBS        := -lsq_terminal -lutil -lpthread
BIN           := sq_shell

OBJ_DIR       := obj
SRCS          := $(shell find src -name "*.c")
OBJS          := $(SRCS:src/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS   := $(patsubst $(ROOT)/src/%.c, $(OBJ_DIR)/common/%.o, $(COMMON_SRCS))

all: $(BIN)

$(BIN): $(OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS_PATHS) $(LDLIBS)

test: $(OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -DDEBUG $^ -o $(BIN)_test $(LDFLAGS_PATHS) $(LDLIBS)

deploy: $(SRCS) $(COMMON_SRCS)
	$(CC) -std=c23 -Wall -Wextra -O2 $(INCLUDES) -DDEPLOY $^ -o $(BIN)_deploy $(LDFLAGS_PATHS) $(LDLIBS)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(ROOT)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

lsp:
	$(call gen_lsp, $(SRCS) $(COMMON_SRCS), compile_commands.json)

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(BIN)_test $(BIN)_deploy compile_commands.json

.PHONY: all clean lsp test deploy
