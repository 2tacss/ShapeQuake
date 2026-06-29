#ifndef SQ_PROTOCOL_H
#define SQ_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

typedef enum sq_packet_type_t sq_packet_type_t;
typedef struct sq_packet_header_t sq_packet_header_t;
typedef struct sq_packet_body_t sq_packet_body_t;

/* MAGIC */
#define SQ_MAGIC 0x51534551  // "SQES" (ShapeQuake Engine Stream)
#define SQ_SOCKET_PATH "/tmp/shapequake.sock"

typedef struct sq_connection {
	int fd;
	bool is_running;
} sq_socket_handle_t;

enum sq_packet_type_t {
	SQ_CAT_EXEC_LOG = 	0x1000,   /* Command execution data and metadata      */
	SQ_CAT_SEND = 		0x2000,   /* Command execution result output          */
	SQ_CAT_HEARTBEAT = 	0x3000,   /* Keep-alive for persistent connections    */
	SQ_CAT_NOTIFY = 	0x4000,   /* General notification message             */
	SQ_CAT_UI = 		0x5000,   /* Node UI Information                      */
	SQ_CAT_SYS_INFO = 	0x7000,   /* System metrics (CPU, Mem) for UI display */
	SQ_CAT_ERR_LOG = 	0xE000,   /* Standard error (stderr) output           */
	SQ_TYPE_OVERVIEW = 	0x2001,
	SQ_TYPE_GEOMETRY = 	0x5001,    // Request for window shape calculation
};

/* Packet Header 12 bytes */
struct sq_packet_header_t {
    uint32_t magic;
    uint32_t type;
    uint32_t payload_size;
};

/* Body: Main Node Command Log  */
struct sq_packet_body_t {
    uint32_t id;
    uint32_t timestamp;
    char command[256];
    char working_dir[256];
    char project_name[64];
};

/**
 * Sub node Context
 * Middleware to Terminal
 * Attributes for node execution (Parsed from shell)
 */
typedef struct {
	uint32_t flags;       /* INTERACTIVE, LOCKED, etc */
	char shape_hint[32];  /* User specified geometry name */
	size_t cmd_len;       /* Length of command_line string */
	char command_line[];  /* Followed by the null-terminated command string */
} sq_payload_exec_t;

/**
 * Progress Metadata
 * Sent periodically within EXEC_RESULT or as a separate STATUS packet.
 */
typedef struct {
	double percentage;    /* 0.0 to 1.0 */
	uint64_t bytes_read;  /* Total bytes processed so far */
	uint32_t line_count;  /* Total lines processed */
	bool is_indeterminate;/* True if total size is unknown (e.g., tail -f) */
} sq_progress_t;

/**
 * Payload for SQ_TYPE_EXEC_RESULT
 * This is sent sequentially as data arrives from PTY.
 */
typedef struct {
	sq_progress_t progress;
	size_t chunk_size;    /* Size of the following data chunk */
	char data[];          /* Raw terminal output (VT100) */
} sq_payload_result_t;


#endif
