#ifndef _REACTOR_H
#define _REACTOR_H

#include "allocator.h"
#include <unistd.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <semaphore.h>


#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

/************
 * Forwards *
 ************/
typedef enum task_type_t task_type_t;
typedef enum event_from_t event_from_t;
typedef enum event_type_t event_type_t;
typedef enum shared_type_t shared_type_t;
typedef struct common_task_t common_task_t;
typedef struct task_t task_t;
typedef struct worker_t worker_t;
typedef struct pool_t pool_t;
typedef struct job_t job_t;
typedef struct job_driver_t job_driver_t;
typedef struct task_shared_data_t task_shared_data_t;
typedef struct job_shared_data_t job_shared_data_t;

/******************
 * Opaque Pointers *
 ******************/
typedef const struct job_t *job_id_t;

#define MAX_POOLS 96
#define MAX_LOCAL_QUEUES 256
#define MAX_JOBS 16
#define MAX_EVENTS 160
#define MAX_JOB_SHARED_DATA 16
#define MAX_TASK_SHARED_DATA 16
#define MAX_SHM 8
#define MAX_SHM_NAME 32

#define THREAD_PROCESS_SHARED 1
#define THREAD_PROCESS_NO_SHARED 0

enum task_type_t {
	TASK_TYPE_SOMETHING
};

enum event_from_t {
	EVENT_FROM_EFD, // eventfd
	EVENT_FROM_SFD, // signalfd
	EVENT_FROM_TFD, // timerfd
	EVENT_FROM_UFD // usbfd
};

enum event_type_t {
	EVENT_TYPE_SIGCHLD
};

enum shared_type_t {
	TYPE_SHARED_WORKER,
	TYPE_SHARED_JOB,
	TYPE_SHARED_BOTH
};

typedef struct {
	const void *data_ptr;
	const void *list[];
} shared_data_t;

struct job_shared_data_t {
	const int shmid;
	const char *shmname;
	const shared_data_t *shared;
};

struct task_shared_data_t {
	const int id;
	const shared_data_t *shared;
};

struct pool_t {
	const int id;
	int worker_count;
	int job_shared_count;
	int task_shared_count;
	worker_t *workers;
	job_t *jobs;
	job_shared_data_t *shared_job_data[MAX_JOB_SHARED_DATA];
	task_shared_data_t *shared_task_data[MAX_TASK_SHARED_DATA];
};

struct common_task_t {
	const int id;
	const int fdev;
	const int fdsig;
	const event_from_t from;
	const event_type_t event_type;
	const task_type_t task_type;
	void *(*execute)(common_task_t *self);
	void *(*on_load)(common_task_t *self);
	void *(*on_exit)(common_task_t *self);
	const void *arg;
};

struct worker_t {
	const int id;
	const pthread_t tid;
	task_t *local_queue[MAX_LOCAL_QUEUES];
	sem_t sem;
	task_shared_data_t *(*shared)(common_task_t *self, int shared_data); 
	void (*notify_done)(common_task_t *self);
};

struct task_t {
	common_task_t super;
	pool_t *pool;
	worker_t *my_worker;
};

struct job_t {
	const int id;
	const char *shmname;
	const event_from_t from;
	const event_type_t event_type;
	const job_shared_data_t *shm;
	const pid_t pid;
	const void *table_callbacks;
	void *arg;

	void *(*on_exit)(int exit_code);
};

struct job_driver_t {
	void *(*spawn)(job_t *job);
	void *(*request_stop)(job_t *job);
	int (*kill)(job_t *job, int signal);
	int (*wait)(job_t *job, int *exit_code);
};

int worker_count(common_task_t *self);
task_shared_data_t *shared(common_task_t *self, int shared_id);
void notify_done(common_task_t *self);
pool_t *reactor_init(heap_tracker_t *tracker, int worker_count, int job_count);
void init_worker(heap_tracker_t *tracker, pool_t *pool, int cur, int id, int pshare);
uint64_t init_job(pool_t *pool);
uint64_t init_shared(shared_type_t shared_type);
task_t *init_task(void);


#endif
