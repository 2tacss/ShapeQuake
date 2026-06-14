include ../config.mk

INCLUDES := -Iinclude $(COMMON_INCLUDES) -I../shell/include
LDLIBS   := -lpthread -lsqlite3
BIN      := sq_server

OBJ_DIR  := obj
SRCS     := $(shell find src -name "*.c")
OBJS     := $(SRCS:src/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS := $(patsubst $(ROOT)/src/%.c, $(OBJ_DIR)/common/%.o, $(COMMON_SRCS))

all: $(BIN)

$(BIN): $(OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(ROOT)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

lsp:
	$(call gen_lsp, $(SRCS) $(COMMON_SRCS), compile_commands.json)

clean:
	rm -rf $(OBJ_DIR) $(BIN) compile_commands.json

.PHONY: all clean lsp
