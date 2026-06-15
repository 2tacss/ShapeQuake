#ifndef TEST_TABLE_H_
#define TEST_TABLE_H_

#include "status.h"

/*
	Conversion Table for `status_t`.
	One liner Nushell code auto generation `case-return value`.
	All of paths are relative.
	
	open ../include/status.h | lines | filter { $in =~ "^\\s*CAT_" } | each { |line| let name = ($line | parse --regex '(?P<n>CAT_[A-Z0-0_]+)' | get n.0); $"case ($name): return \"($name)\";" } | str join (char nl) | clip
	open ../include/status.h | lines | filter { $in =~ "^\\s*CND_" } | each { |line| let name = ($line | parse --regex '(?P<n>CND_[A-Z0-0_]+)' | get n.0); $"case ($name): return \"($name)\";" } | str join (char nl) | clip
	open ../include/status.h | lines | filter { $in =~ "^\\s*CODE_" } | each { |line| let name = ($line | parse --regex '(?P<n>CODE_[A-Z0-0_]+)' | get n.0); $"case ($name): return \"($name)\";" } | str join (char nl) | clip
*/

static inline const char *status_cat_name(cat_t cat) {
	switch (cat) {
		case CAT_NONE: return "CAT_NONE";
		case CAT_SHELL: return "CAT_SHELL";
		case CAT_TERMINAL: return "CAT_TERMINAL";
		case CAT_NODE_MAIN: return "CAT_NODE_MAIN";
		case CAT_NODE_MAIN_OVERVIEW: return "CAT_NODE_MAIN_OVERVIEW";
		case CAT_NODE_SUB: return "CAT_NODE_SUB";
		case CAT_SHAPE: return "CAT_SHAPE";
		case CAT_PROG_LINE: return "CAT_PROG_LINE";
		case CAT_COLOR: return "CAT_COLOR";
		case CAT_ARENA: return "CAT_ARENA";
		case CAT_RESPONSE: return "CAT_RESPONSE";
		case CAT_VALUE: return "CAT_VALUE";
		case CAT_SERVER: return "CAT_SERVER";
		case CAT_CLIENT: return "CAT_CLIENT";
		case CAT_MAINFRAME: return "CAT_MAINFRAME";
		case CAT_PROCESS: return "CAT_PROCESS";
		case CAT_THREAD: return "CAT_THREAD";
		case CAT_HEAP: return "CAT_HEAP";
		case CAT_HEAP_TRACKER: return "CAT_HEAP_TRACKER";
		case CAT_VMA: return "CAT_VMA";
		case CAT_REACTOR: return "CAT_REACTOR";
		case CAT_JOB: return "CAT_JOB";
		case CAT_LOGGER: return "CAT_LOGGER";
		case CAT_VIEW: return "CAT_VIEW";
	}
}

static inline const char *status_cnd_name(cnd_t cnd) {
	switch (cnd) {
		case CND_SUCCESS: return "CND_SUCCESS";
		case CND_FAILURE: return "CND_FAILURE";
		case CND_INFO: return "CND_INFO";
		case CND_DEBUG: return "CND_DEBUG";
		case CND_WARN: return "CND_WARN";
		case CND_FATAL: return "CND_FATAL";
		case CND_REQUIRE: return "CND_REQUIRE";
		case CND_REFUSE: return "CND_REFUSE";
		case CND_DENIED: return "CND_DENIED";
		case CND_NULL: return "CND_NULL";
		case CND_INVALID: return "CND_INVALID";
		case CND_INTERRUPTION: return "CND_INTERRUPTION";
		case CND_ABORT: return "CND_ABORT";
		case CND_WAIT: return "CND_WAIT";
		case CND_DEAD: return "CND_DEAD";
		case CND_RETRY: return "CND_RETRY";
	}
}

static inline const char *status_code_name(code_t code) {
	switch(code) {
		case CODE_PARAM: return "CODE_PARAM";
		case CODE_SIZE: return "CODE_SIZE";
		case CODE_MAGIC: return "CODE_MAGIC";
		case CODE_CONNECTION: return "CODE_CONNECTION";
		case CODE_FILE: return "CODE_FILE";
		case CODE_DESC: return "CODE_DESC";
		case CODE_SET: return "CODE_SET";
		case CODE_VALUE: return "CODE_VALUE";
		case CODE_SEND: return "CODE_SEND";
		case CODE_RECV: return "CODE_RECV";
		case CODE_OPEN: return "CODE_OPEN";
		case CODE_CLOSE: return "CODE_CLOSE";
		case CODE_READ: return "CODE_READ";
		case CODE_WRITE: return "CODE_WRITE";
		case CODE_ALLOC: return "CODE_ALLOC";
		case CODE_CLEAR: return "CODE_CLEAR";
		case CODE_DESTROY: return "CODE_DESTROY";
		case CODE_FREE: return "CODE_FREE";
		case CODE_CALCULATION: return "CODE_CALCULATION";
		case CODE_EXIST: return "CODE_EXIST";
		case CODE_NO_EXIST: return "CODE_NO_EXIST";
		case CODE_FOUND: return "CODE_FOUND";
		case CODE_NOT_FOUND: return "CODE_NOT_FOUND";
		case CODE_RANGE: return "CODE_RANGE";
		case CODE_EXIT: return "CODE_EXIT";
		case CODE_JOIN: return "CODE_JOIN";
	}
}

#endif
