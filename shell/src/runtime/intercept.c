#include "runtime/interceptor.h"
#include "defines.h"

bool sq_intercept_input(byte b) {
	if (b == SQ_KEY_CTRL_Q) {
		return false;
	}
	return true;
}

void sq_intercept_output(byte b) {
	// Currently, we just let the byte pass through to the parser.
	// This is where we could "listen" for specific shell prompts
	// to trigger UI state changes.
	(void)b; // SQ_MAYBE_UNUSED logic
}
