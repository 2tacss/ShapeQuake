#include "runtime/context.h"
#include <string.h>

void shell_context_init(shell_context_t *ctx) {
	if (!ctx) return;

	memset(ctx, 0, sizeof(shell_context_t));
	__asm__ volatile("" : : : "memory");
	atomic_init(&ctx->state, SHELL_STATE_IDLE);
	atomic_init(&ctx->foreground_pid, -1);
}

void shell_context_set_state(shell_context_t *ctx, shell_state_t state) {
	if (!ctx) return;

	// another flag `memory_order_seq_cst` is restrictly for processing sequences
	atomic_store_explicit(&ctx->state, state, memory_order_relaxed);
}

shell_state_t shell_context_get_state(shell_context_t *ctx) {
	if (!ctx) return SHELL_STATE_IDLE;

	return atomic_load_explicit(&ctx->state, memory_order_relaxed);
}
