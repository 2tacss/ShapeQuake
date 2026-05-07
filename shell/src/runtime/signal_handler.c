#include "runtime/signal_handler.h"
#include "defines.h"
#include "signal.h"
#include <signal.h>
#include <stddef.h>

volatile sig_atomic_t g_stop_requested = SIGNAL_FALSE;
volatile sig_atomic_t g_window_resized = SIGNAL_FALSE;

static void handle_signal(int sig) {
	switch (sig) {
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			g_stop_requested = SIGNAL_TRUE;
			break;
		case SIGWINCH:
			g_window_resized = SIGNAL_TRUE;
			break;
		default:
			break;
	}
}

void setup_signal_handler(void) {
	struct sigaction sa = {0};
	
	sa.sa_handler = handle_signal;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask); 

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	sigaction(SIGWINCH, &sa, NULL);

	sigaction(SIGHUP, &sa, NULL);
}
