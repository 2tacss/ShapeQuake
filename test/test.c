#include "test.h"
#include "status.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void dbgmsg(debug_meta_t *meta) {
	if (!meta) return;
	if (!meta || 1 > meta->at_line) return;
	if (!meta->extract) return;

	char buffer[MAX_PROMPT_BUFFER];
	const char *body = prompt_body(meta, buffer, MAX_PROMPT_BUFFER);
	print_(body);
}

void print_(const char *prompt_buffer) {
	size_t remains = strlen(prompt_buffer);
	const char *ptr = prompt_buffer;
	while (remains > 0) {
		ssize_t s = write(STDERR_FILENO, ptr, remains);
		if (s < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				break;
			}
		}
		ptr += s;
		remains -= (size_t)s;
	}
}

const char *prompt_body(debug_meta_t *meta, char *dst, size_t buffer_size) {
	if (!meta || 1 > meta->at_line || !dst) return nullptr;
	if (!meta->extract) return nullptr;

	char msg[MAX_MSG] = {0};
	char funcname[MAX_FUNC_NAME] = {0};
	char trouble_source[MAX_MSG] = {0};
	
	strncpy(msg, meta->msg, MAX_MSG);
	strncpy(funcname, meta->at_func, MAX_FUNC_NAME);
	strncpy(trouble_source, meta->trouble_source, MAX_MSG);

	status_extracted_t status_extracted = {0};
	meta->extract(&status_extracted, meta->status);

	const char *prompt =
	"=================================================================\n"
	" DEBUG META\n"
	"=================================================================\n"
	"%s STATUS:\n"
	"\t0x%016llX - %s\n"
	"\t0x%016llX - %s\n"
	"\t0x%016llX - %s\n"
	"%s DETAIL:\n"
	"\t%ld in %s\n"
	"\tFunc: %s\n"
	"\tDetail: %s\n"
	"\t%s\n"
	;

	uint64_t category = status_extracted.category & 0xFFFF000000000000ULL;
	uint64_t condition = status_extracted.condition & 0x0000FFFF00000000ULL;
	uint64_t code = status_extracted.code & 0x00000000FFFFFFFFULL;
	
	snprintf(dst, buffer_size, prompt,
		PROMPT_MARK_SIGN,
		category,
		status_extracted.name_category,
		condition,
		status_extracted.name_condition,
		code,
		status_extracted.name_code,
		PROMPT_MARK_PLUS,
		meta->at_line,
		meta->at_filename,
		meta->at_func,
		meta->trouble_source,
		meta->msg
	);
	meta->prompt_buffer = dst;
	return dst;
}

void is_called(void) {
	debug_meta_t meta = DEBUG_META(asstatus(CAT_VALUE, CND_SUCCESS, CODE_CONNECTION), "DEBUG_META()", "can you see this?");
	dbgmsg(&meta);
}

