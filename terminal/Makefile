include ../config.mk

INCLUDES  := -Iinclude \
             -Iinclude/pty \
             -Iinclude/vterm \
             $(COMMON_INCLUDES) \
             $(shell pkg-config --cflags vterm)

LDFLAGS   := $(shell pkg-config --libs vterm) -lutil -lpthread

OBJ_DIR   := obj
BIN       := terminal_test
LIB_NAME  := libsq_terminal.a

ALL_SRCS  := $(shell find src -name "*.c")
SRCS      := $(filter-out src/main.c, $(ALL_SRCS))

OBJS      := $(SRCS:src/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS := $(patsubst $(ROOT)/src/%.c, $(OBJ_DIR)/common/%.o, $(COMMON_SRCS))

lib: $(OBJS) $(COMMON_OBJS)
	ar rcs $(LIB_NAME) $^

test: $(ALL_SRCS) $(COMMON_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $(BIN) $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(ROOT)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

lsp:
	$(call gen_lsp, $(ALL_SRCS) $(COMMON_SRCS), compile_commands.json)

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(LIB_NAME) compile_commands.json

.PHONY: lib test lsp clean
