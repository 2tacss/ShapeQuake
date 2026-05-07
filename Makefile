CC        := clang
CFLAGS    := -std=c23 -Wall -Wextra -g3 -O0 \
             -Iinclude \
             -I../include \
             -I../shell/include \
             -D_POSIX_C_SOURCE=200809L \
             -DDEBUG

LDLIBS    := -lpthread

BIN       := ../sq_server

SRC_DIR     := src
SERVER_SRCS := $(shell find $(SRC_DIR) -name "*.c")

SHARED_SRCS := ../shell/src/runtime/allocator.c

ALL_SRCS := $(SERVER_SRCS) $(SHARED_SRCS)

all: $(BIN)

$(BIN): $(ALL_SRCS)
	$(CC) $(CFLAGS) $(ALL_SRCS) -o $(BIN) $(LDLIBS)

lsp:
	@printf "[\n" > compile_commands.json
	@first=1; \
	for src in $(ALL_SRCS); do \
		if [ $$first -ne 1 ]; then printf ",\n" >> compile_commands.json; fi; \
		printf "  {\n" >> compile_commands.json; \
		printf "    \"directory\": \"$(CURDIR)\",\n" >> compile_commands.json; \
		printf "    \"command\": \"$(CC) $(CFLAGS) -c $$src\",\n" >> compile_commands.json; \
		printf "    \"file\": \"$$src\"\n" >> compile_commands.json; \
		printf "  }" >> compile_commands.json; \
		first=0; \
	done
	@printf "\n]\n" >> compile_commands.json
	@echo "Generated server/compile_commands.json"

clean:
	rm -f $(BIN) compile_commands.json

.PHONY: all clean lsp
