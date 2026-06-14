#include "terminal.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <vterm.h>
#include <pty.h>
#include "pty/sq_pty.h"

static void *pty_read_thread(void *arg) {
	sq_terminal_t *term = (sq_terminal_t *)arg;
	byte buffer[4096];
	ssize_t n;

	while (term->is_running) {
		n = read(term->pty_master, buffer, sizeof(buffer));
		if (n <= 0) {
			term->is_running = false;
			break;
		}
		vterm_input_write(term->vt, (const char *)buffer, n);
	}
	return NULL;
}

sq_terminal_t *sq_terminal_create(uint32_t rows, uint32_t cols) {
	sq_terminal_t *term = malloc(sizeof(sq_terminal_t));
	if (!term) return NULL;

	term->vt = vterm_new(rows, cols);
	term->is_running = true;
	term->pty_master = -1;

	VTermScreen *screen = vterm_obtain_screen(term->vt);
	vterm_screen_reset(screen, 1);

	return term;
}

int sq_terminal_spawn(sq_terminal_t *term, char **argv) {
	if (!term) return -1;

	pid_t pid = forkpty(&term->pty_master, NULL, NULL, NULL);
	if (pid < 0) return -1;

	if (pid == 0) {
		execvp(argv[0], argv);
		exit(1);
	}

	if (pthread_create(&term->read_thread, NULL, pty_read_thread, term) != 0) {
		return -1;
	}

	return 0;
}

void sq_terminal_write_byte(sq_terminal_t *term, byte b) {
	if (term && term->pty_master != -1) {
		write(term->pty_master, &b, 1);
	}
}

void sq_terminal_destroy(sq_terminal_t *term) {
	if (!term) return;

	term->is_running = false;
	if (term->pty_master != -1) {
		close(term->pty_master);
		pthread_join(term->read_thread, NULL);
	}

	vterm_free(term->vt);
	free(term);
}
