#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <stdint.h>
#include "pty/sq_pty.h"

int main(void) {
	/* Initialize PTY without vterm dependency */
	sq_pty_t* pty = sq_pty_create();
	if (!pty) {
		fprintf(stderr, "Failed to create pty\n");
		return 1;
	}

	char *argv[] = {"/bin/sh", NULL};
	/* Spawn shell process within the PTY */
	if (sq_pty_spawn(pty, argv) != 0) {
		fprintf(stderr, "Failed to spawn shell\n");
		return 1;
	}

	printf("--- Terminal PTY Test (Zen Mode). Ctrl+C to exit ---\n");

	struct pollfd fds[2];
	/* 0: Standard Input (keyboard) */
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	/* 1: PTY Master (output from shell) */
	fds[1].fd = pty->fd;
	fds[1].events = POLLIN;

	while (true) {
		/* Wait for events on either file descriptor */
		if (poll(fds, 2, -1) < 0) break;

		/* Handle keyboard input -> Write to PTY */
		if (fds[0].revents & POLLIN) {
			byte b;
			if (read(STDIN_FILENO, &b, 1) > 0) {
				sq_pty_write_byte(pty, b); 
			}
		}

		/* Handle PTY output -> Write to Screen (STDOUT) */
		if (fds[1].revents & POLLIN) {
			char buf[4096];
			ssize_t n = read(pty->fd, buf, sizeof(buf));
			if (n <= 0) break; /* Exit if child process closes or error occurs */
			write(STDOUT_FILENO, buf, n);
			fflush(stdout);
		}
	}

	sq_pty_destroy(pty);
	return 0;
}
