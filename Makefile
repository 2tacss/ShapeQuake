include config.mk

BIN      := shapequake
INCLUDES := $(COMMON_INCLUDES)

SRCS     := $(ROOT)/main.c $(COMMON_SRCS)
LDLIBS   := -lpthread -lutil

ifneq ($(filter main,$(MAKECMDGOALS)),)
endif

ifneq ($(filter server check-leak,$(MAKECMDGOALS)),)
    INCLUDES += -I$(ROOT)/server/include -I$(ROOT)/shell/include
    SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/server/src/*.c) $(wildcard $(ROOT)/server/src/**/*.c))
    LDLIBS   += -lsqlite3
endif

ifneq ($(filter shell check-leak,$(MAKECMDGOALS)),)
    INCLUDES += -I$(ROOT)/shell/include -I$(ROOT)/terminal/include/pty
	SRCS     += $(filter-out %/main.c, $(shell find $(ROOT)/shell/src -name "*.c"))
#	SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/shell/src/*.c) $(wildcard $(ROOT)/shell/src/**/*.c) $(wildcard $(ROOT)/shell/src/**/**/*.c))
endif

ifneq ($(filter terminal check-leak,$(MAKECMDGOALS)),)
    INCLUDES += -I$(ROOT)/terminal/include -I$(ROOT)/terminal/include/pty $(shell pkg-config --cflags vterm 2>/dev/null || echo "")
    SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/terminal/src/*.c) $(wildcard $(ROOT)/terminal/src/**/*.c))
    LDLIBS   += $(shell pkg-config --libs vterm 2>/dev/null || echo "-lvterm")
endif

ifneq ($(filter ui check-leak,$(MAKECMDGOALS)),)
    INCLUDES += -I$(ROOT)/ui/include -I$(ROOT)/shell/include -I$(ROOT)/terminal/include
    SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/ui/src/*.c) $(wildcard $(ROOT)/ui/src/**/*.c))
    LDLIBS   += -lX11 -lXext -lXrender
endif

ifneq ($(filter test,$(MAKECMDGOALS)),)
    INCLUDES += -I$(ROOT)
    SRCS     += $(wildcard $(ROOT)/test/*.c)
endif

ifneq ($(filter check-leak,$(MAKECMDGOALS)),)
    INCLUDES += -I$(ROOT)
    SRCS     += $(wildcard $(ROOT)/test/*.c)
endif

ifeq ($(MAKECMDGOALS),)
    MAKECMDGOALS := main server shell terminal ui test
    include $(ROOT)/Makefile
endif

ifneq ($(filter lsp,$(MAKECMDGOALS)),)
    INCLUDES += -I$(ROOT) \
                -I$(ROOT)/server/include \
                -I$(ROOT)/shell/include \
                -I$(ROOT)/terminal/include \
                -I$(ROOT)/terminal/include/pty \
                -I$(ROOT)/ui/include \
                $(shell pkg-config --cflags vterm 2>/dev/null || echo "")

    SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/server/src/*.c) $(wildcard $(ROOT)/server/src/**/*.c)) \
                $(filter-out %/main.c, $(shell find $(ROOT)/shell/src -name "*.c" 2>/dev/null || echo "")) \
                $(filter-out %/main.c, $(wildcard $(ROOT)/terminal/src/*.c) $(wildcard $(ROOT)/terminal/src/**/*.c)) \
                $(filter-out %/main.c, $(wildcard $(ROOT)/ui/src/*.c) $(wildcard $(ROOT)/ui/src/**/*.c)) \
                $(wildcard $(ROOT)/test/*.c)
endif

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDLIBS)

main server shell terminal ui test: $(BIN)
	@:

lsp:
	$(call gen_lsp, $(SRCS), compile_commands.json)

clean:
	rm -f $(BIN) compile_commands.json
	@for dir in server shell terminal ui test; do \
		if [ -f $$dir/Makefile ]; then $(MAKE) -C $$dir clean; fi; \
	done

check-leak:
	$(CC) $(CFLAGS) -std=c23 -fsanitize=address -g $(INCLUDES) $(SRCS) -o $(BIN)_leak $(LDLIBS); ./$(BIN)_leak; rm -f $(BIN)_leak

.PHONY: all clean lsp main server shell terminal ui test check-leak
