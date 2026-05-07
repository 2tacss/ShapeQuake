#ifndef SQ_EVENTS_H_
#define SQ_EVENTS_H_

/* DB Events */
typedef enum {
	INSERT,
	SELECT,
} sq_event_type_db_t;

/* Common Events */
typedef enum {
	DB,
	UI,
} sq_event_type_common_t;


#endif
