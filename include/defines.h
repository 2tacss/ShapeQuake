#ifndef SHAPEQUAKE_DEFINES_H
#define SHAPEQUAKE_DEFINES_H

#include <stdint.h>

/**
 * Terminal Emulation Constants
 */
#define SQ_DEFAULT_ROWS 24
#define SQ_DEFAULT_COLS 80

/**
 * Buffer Sizes
 */
#define SQ_LINE_BUF_SIZE 1024
#define SQ_PTY_BUF_SIZE  4096

/**
 * Data Sizez
 */
typedef uint16_t sq_u16_t;
typedef uint8_t byte;


/**
 * Special Key Codes (ASCII/Raw Mode)
 */
#define SQ_KEY_CTRL_Q 0x11
#define SQ_KEY_BACKSPACE 0x7F
#define SQ_KEY_DELETE 0x08
#define SQ_KEY_ENTER  0x0D

#define PIPE_FD_COUNT 2
#define PIPE_FD_READ 0
#define PIPE_FD_WRITE 1

#define BUFFER_SIZE_COMMAND 1024


#define SIGNAL_TRUE 1
#define SIGNAL_FALSE 0

#endif // SHAPEQUAKE_DEFINES_H
