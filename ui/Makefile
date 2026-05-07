include ../config.mk

LIB_NAME := libsq_ui.a
BIN      := shapequake-ui

INCLUDES := -Iinclude \
            $(COMMON_INCLUDES) \
            -I../shell/include \
            -I../terminal/include
LDFLAGS  := -lX11 -lXext -lXrender

OBJ_DIR  := obj
SRCS     := $(shell find src -name "*.c")
OBJS     := $(SRCS:src/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS := $(patsubst $(ROOT)/src/%.c, $(OBJ_DIR)/common/%.o, $(COMMON_SRCS))

.PHONY: all clean lsp lib

all: $(BIN)

lib: $(LIB_NAME)

$(BIN): $(OBJS) $(COMMON_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(LIB_NAME): $(OBJS) $(COMMON_OBJS)
	ar rcs $@ $^

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(ROOT)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

lsp:
	$(call gen_lsp, $(SRCS) $(COMMON_SRCS), compile_commands.json)

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(LIB_NAME) compile_commands.json
