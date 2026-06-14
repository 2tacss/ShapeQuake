include config.mk

BIN      := shapequake
INCLUDES := $(COMMON_INCLUDES)
SRCS     := $(ROOT)/main.c $(COMMON_SRCS)
LDLIBS   := -lpthread -lutil

ifneq ($(filter main,$(MAKECMDGOALS)),)
endif

ifneq ($(filter server,$(MAKECMDGOALS)),)
	INCLUDES += -I$(ROOT)/server/include -I$(ROOT)/shell/include
	SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/server/src/*.c) $(wildcard $(ROOT)/server/src/**/*.c))
	LDLIBS   += -lsqlite3
endif

ifneq ($(filter shell,$(MAKECMDGOALS)),)
	INCLUDES += -I$(ROOT)/shell/include -I$(ROOT)/terminal/include/pty
	SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/shell/src/*.c) $(wildcard $(ROOT)/shell/src/**/*.c))
endif

ifneq ($(filter terminal,$(MAKECMDGOALS)),)
	INCLUDES += -I$(ROOT)/terminal/include -I$(ROOT)/terminal/include/pty $(shell pkg-config --cflags vterm 2>/dev/null || echo "")
	SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/terminal/src/*.c) $(wildcard $(ROOT)/terminal/src/**/*.c))
	LDLIBS   += $(shell pkg-config --libs vterm 2>/dev/null || echo "-lvterm")
endif

ifneq ($(filter ui,$(MAKECMDGOALS)),)
	INCLUDES += -I$(ROOT)/ui/include -I$(ROOT)/shell/include -I$(ROOT)/terminal/include
	SRCS     += $(filter-out %/main.c, $(wildcard $(ROOT)/ui/src/*.c) $(wildcard $(ROOT)/ui/src/**/*.c))
	LDLIBS   += -lX11 -lXext -lXrender
endif

ifneq ($(filter test,$(MAKECMDGOALS)),)
	INCLUDES += -I$(ROOT)/test
	SRCS     += $(wildcard $(ROOT)/test/*.c)
endif

ifeq ($(MAKECMDGOALS),)
	MAKECMDGOALS := main server shell terminal ui test
	include $(ROOT)/Makefile
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

.PHONY: all clean lsp main server shell terminal ui test
