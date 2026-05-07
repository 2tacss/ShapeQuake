# config.mk
CC      := clang
CFLAGS  := -std=c23 -Wall -Wextra -g3 -O0 -D_POSIX_C_SOURCE=200809L -DDEBUG
ROOT    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

COMMON_INCLUDES := -I$(ROOT)/include \
                   -I$(ROOT)/include/error

COMMON_SRCS := $(ROOT)/src/allocator.c \
               $(ROOT)/src/error/sq_error.c

define gen_lsp
	@printf "[\n" > $(2)
	@first=1; \
	for src in $(1); do \
		if [ $$first -ne 1 ]; then printf ",\n" >> $(2); fi; \
		printf "  {\n" >> $(2); \
		printf "    \"directory\": \"$(CURDIR)\",\n" >> $(2); \
		printf "    \"command\": \"$(CC) $(CFLAGS) $(INCLUDES) -c $$src\",\n" >> $(2); \
		printf "    \"file\": \"$$src\"\n" >> $(2); \
		printf "  }" >> $(2); \
		first=0; \
	done
	@printf "\n]\n" >> $(2)
	@echo "Generated $(CURDIR)/$(2)"
endef
