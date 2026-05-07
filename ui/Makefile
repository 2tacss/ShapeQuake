LIB_NAME := libsq_ui.a
BIN      := shapequake-ui
CC       := clang

CFLAGS   := -std=c23 -Wall -Wextra -g3 -O0 \
            -Iinclude \
            -I../include \
            -I../shell/include \
            -I../terminal/include \
            -D_POSIX_C_SOURCE=200809L
LDFLAGS  := -lX11 -lXext -lXrender

SRC_DIR  := src
OBJ_DIR  := obj

SHARED_SRC := ../src/allocator.c
SRCS       := $(shell find $(SRC_DIR) -name "*.c")
OBJS       := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean lsp lib

all: $(BIN)

lib: $(LIB_NAME) $(OBJ_DIR)/allocator.o
	ar rcs $(LIB_NAME) $(OBJS) $(OBJ_DIR)/allocator.o

$(BIN): $(OBJS) $(OBJ_DIR)/allocator.o
	$(CC) $(OBJS) $(OBJ_DIR)/allocator.o -o $@ $(LDFLAGS)

$(LIB_NAME): $(OBJS) $(OBJ_DIR)/allocator.o
	ar rcs $@ $(OBJS) $(OBJ_DIR)/allocator.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/allocator.o: $(SHARED_SRC)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

lsp:
	@printf "[\n" > compile_commands.json
	@first=1; \
	for src in $(SRCS) $(SHARED_SRC); do \
		if [ $$first -ne 1 ]; then printf ",\n" >> compile_commands.json; fi; \
		printf "  {\n" >> compile_commands.json; \
		printf "    \"directory\": \"$(CURDIR)\",\n" >> compile_commands.json; \
		printf "    \"command\": \"$(CC) $(CFLAGS) -c $$src\",\n" >> compile_commands.json; \
		printf "    \"file\": \"$$src\"\n" >> compile_commands.json; \
		printf "  }" >> compile_commands.json; \
		first=0; \
	done
	@printf "\n]\n" >> compile_commands.json
	@echo "Generated ui/compile_commands.json"

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(LIB_NAME) compile_commands.json
