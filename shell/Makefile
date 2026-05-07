CC        := clang
CFLAGS    := -std=c23 -Wall -Wextra -g3 -O0 \
             -Iinclude \
             -I../include \
             -I../terminal/include/pty \
             -D_POSIX_C_SOURCE=200809L

LDFLAGS_PATHS := -L../terminal
LDLIBS        := -lsq_terminal -lutil -lpthread

BIN           := sq_shell

SRC_DIR       := src
SHARED_SRC    := ../src/allocator.c
ALL_SRCS      := $(shell find $(SRC_DIR) -name "*.c") $(SHARED_SRC)

all: $(BIN)

$(BIN): $(ALL_SRCS)
	$(CC) $(CFLAGS) -DDEBUG $(ALL_SRCS) -o $(BIN) $(LDFLAGS_PATHS) $(LDLIBS)

test: $(ALL_SRCS)
	$(CC) $(CFLAGS) -DDEBUG $(ALL_SRCS) -o $(BIN)_test $(LDFLAGS_PATHS) $(LDLIBS)

deploy: $(ALL_SRCS)
	$(CC) $(CFLAGS) -O2 -DDEPLOY $(ALL_SRCS) -o $(BIN)_deploy $(LDFLAGS_PATHS) $(LDLIBS)

lsp:
	@printf "[\n" > compile_commands.json
	@first=1; \
	for src in $(ALL_SRCS); do \
		if [ $$first -ne 1 ]; then printf ",\n" >> compile_commands.json; fi; \
		printf "  {\n" >> compile_commands.json; \
		printf "    \"directory\": \"$(CURDIR)\",\n" >> compile_commands.json; \
		printf "    \"command\": \"$(CC) $(CFLAGS) -DDEBUG -c $$src\",\n" >> compile_commands.json; \
		printf "    \"file\": \"$$src\"\n" >> compile_commands.json; \
		printf "  }" >> compile_commands.json; \
		first=0; \
	done
	@printf "\n]\n" >> compile_commands.json
	@echo "Generated shell/compile_commands.json"

clean:
	rm -f $(BIN) $(BIN)_test $(BIN)_deploy compile_commands.json

.PHONY: all clean lsp test deploy
