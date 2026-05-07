#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include "common.h"
#include <signal.h>
#include <stdbool.h>

extern volatile sig_atomic_t g_stop_requested;
extern volatile sig_atomic_t g_window_resized;

void setup_signal_handler(void);

#endif
