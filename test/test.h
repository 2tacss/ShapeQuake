#ifndef TEST_H_
#define TEST_H_

#include "status.h"
#include "table.h"
#include <string.h>

#define DEBUG_META(status_val, trbl_src, message) (debug_meta_t){ \
	.at_line          = __LINE__, \
	.at_filename      = get_relative_path(__FILE__), \
	.at_func          = __func__, \
	.trouble_source   = trbl_src, \
	.msg              = message, \
	.status           = status_val, \
	.prompt_buffer    = nullptr, \
	.extracted_status = nullptr,  \
	.extract          = extract \
};

#define MAX_MSG 128
#define MAX_FILE_NAME 32
#define MAX_FUNC_NAME 16
#define MAX_PROMPT_BUFFER (size_t)2048
#define PROMPT_MARK_SIGN "[ * ]"
#define PROMPT_MARK_MINUX "[ - ]"
#define PROMPT_MARK_PLUS "[ + ]"
#define PROMPT_MARK_TEMPL "[ %s ]"

typedef struct debug_meta_t debug_meta_t;
typedef struct status_extracted_t status_extracted_t;

struct debug_meta_t {
	unsigned long at_line;
	const char *at_filename;
	const char *at_func;
	const char *trouble_source;
	const char *msg;
	const status_t status;
	char *prompt_buffer;
	status_extracted_t *extracted_status;
	status_extracted_t (*extract)(status_t status);
};

struct status_extracted_t {
	cat_t category;
	cnd_t condition;
	code_t code;
	const char *name_category;
	const char *name_condition;
	const char *name_code;
};

void dbgmsg(debug_meta_t *meta);
void print_(const char *prompt_buffer);
const char *prompt_body(debug_meta_t *meta, char *dst, size_t buffer_size);
static inline const char* get_relative_path(const char *filepath);

// calling test from root/main.c
// printf message
void is_called(void);

static inline status_extracted_t extract(status_t status) {
	cat_t cat = get_cat(status);
	cnd_t cnd = get_cnd(status);
	code_t code = get_code(status);

	return (status_extracted_t){
		.category = cat,
		.condition = cnd,
		.code = code,
		.name_category = status_cat_name(cat),
		.name_condition = status_cnd_name(cnd),
		.name_code = status_code_name(code)
	};
}

static inline const char* get_relative_path(const char *filepath) {
	if (!filepath) return "";
	
	const char *p = filepath + strlen(filepath);
	int slash_count = 0;
	
	while (p > filepath) {
		p--;
		if (*p == '/') {
			slash_count++;
			if (slash_count == 2) {
				return p + 1;
			}
		}
	}
	return filepath;
}


#endif
