#include "sq_pty.h"
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>

sq_pty_t* sq_pty_create(void) {
	sq_pty_t *pty = calloc(1, sizeof(sq_pty_t));
	if (!pty) return NULL;
	pty->fd = -1;
	return pty;
}

int sq_pty_spawn(sq_pty_t *pty, char **argv) {
	/* forkpty takes &pty->fd to store the master file descriptor */
	pty->pid = forkpty(&pty->fd, NULL, NULL, NULL);
	if (pty->pid < 0) return -1;

	if (pty->pid == 0) {
		execvp(argv[0], argv);
		exit(1);
	}
	return 0;
}

void sq_pty_write_byte(sq_pty_t *pty, byte b) {
	if (pty && pty->fd != -1) {
		write(pty->fd, &b, 1);
	}
}

void sq_pty_destroy(sq_pty_t *pty) {
	if (!pty) return;
	if (pty->fd != -1) close(pty->fd);
	free(pty);
}
