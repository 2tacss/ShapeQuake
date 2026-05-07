CC        := clang
CFLAGS    := -std=c23 -Wall -Wextra -g3 -O0 \
             -Iinclude \
             -Iinclude/pty \
             -Iinclude/vterm \
             -I../include \
             -D_POSIX_C_SOURCE=200809L \
             $(shell pkg-config --cflags vterm)

LDFLAGS   := $(shell pkg-config --libs vterm) -lutil -lpthread

SRC_DIR   := src
OBJ_DIR   := obj
BIN       := terminal_test
LIB_NAME  := libsq_terminal.a

ALL_SRCS  := $(shell find $(SRC_DIR) -name "*.c")
SHARED_SRC := ../src/allocator.c
SRCS      := $(filter-out $(SRC_DIR)/main.c, $(ALL_SRCS))
OBJS      := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

lib: $(OBJS) $(OBJ_DIR)/allocator.o
	ar rcs $(LIB_NAME) $(OBJS) $(OBJ_DIR)/allocator.o

test: $(ALL_SRCS) $(SHARED_SRC)
	$(CC) $(CFLAGS) -DDEBUG $(ALL_SRCS) $(SHARED_SRC) -o $(BIN) $(LDFLAGS)

lsp:
	@printf "[\n" > compile_commands.json
	@first=1; \
	for src in $(ALL_SRCS) $(SHARED_SRC); do \
		if [ $$first -ne 1 ]; then printf ",\n" >> compile_commands.json; fi; \
		printf "  {\n" >> compile_commands.json; \
		printf "    \"directory\": \"$(CURDIR)\",\n" >> compile_commands.json; \
		printf "    \"command\": \"$(CC) $(CFLAGS) -DDEBUG -c $$src\",\n" >> compile_commands.json; \
		printf "    \"file\": \"$$src\"\n" >> compile_commands.json; \
		printf "  }" >> compile_commands.json; \
		first=0; \
	done
	@printf "\n]\n" >> compile_commands.json
	@echo "Generated terminal/compile_commands.json"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/allocator.o: $(SHARED_SRC)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(LIB_NAME) compile_commands.json

.PHONY: lib test lsp clean
