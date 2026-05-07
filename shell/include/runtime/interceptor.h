#ifndef SHAPEQUAKE_INTERCEPTOR_H
#define SHAPEQUAKE_INTERCEPTOR_H

#include "common.h"

/**
 * Evaluates a single byte input from the user.
 * * @param b The raw byte received from stdin.
 * @return true to continue processing, false to initiate shutdown.
 */
SQ_NODISCARD
bool sq_intercept_input(byte b);

/**
 * Evaluates a single byte output from the process/shell.
 * * @param b The raw byte received from the PTY/Shell.
 */
void sq_intercept_output(byte b);

#endif // SHAPEQUAKE_INTERCEPTOR_H
