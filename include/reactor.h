#ifndef _REACTOR_H
#define _REACTOR_H



	/* ===================================================================================== *
	*   Memory Managements                                                                   *
	*  ===================================================================================== *
	*  The structs are managed by three type of memory manager.
	*  💡 Free and destroy manually via `destroy_...()` functions.
	*
	*  ╭─ [ by `heap_tracker` ]
	*  │
	*  ├────── pool_t pool;
	*  ├────── job_t    **jobs;    // pool_t
	*  ╰────── worker_t **workers; // pool_t
	*
	*  ╭─ [ by `vma_alloc` ]
	*  │
	*  ├────── shared_task_data_t   // pool_t
	*  ╰────── shared_job_data_t   // pool_t
	*  < vma uses heap_tracker inside >
	*
	*  ╭─ [ by `arena` ]
	*  │ 
	*  ╰─┬─ task_t *local_queue[MAX_LOCAL_QUEUES]; // worker_t
	*    │
	*    ├───── task_t         task;
	*    ╰───── common_task_t  task_;
	* ========================================================================================= */

#include "heap.h"
#include "vma.h"
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
typedef struct shared_data_t shared_data_t;
typedef struct shared_task_data_t shared_task_data_t;
typedef struct shared_job_data_t shared_job_data_t;

/******************
 * Opaque Pointers *
 ******************/
typedef const struct job_t *job_id_t;

#define MAX_POOLS 96
#define MAX_LOCAL_QUEUES 256
#define MAX_JOBS 16
#define MAX_EVENTS 160
#define MAX_SHARED_TASK_DATA 16
#define MAX_SHARED_JOB_DATA 16
#define MAX_SHM 8
#define MAX_SHM_NAME 32
#define SHARED_DATA_COUNT_NONE -1
#define INVALID_ID -1

#define THREAD_PROCESS_SHARED 1
#define THREAD_PROCESS_SHARED_NO 0

#define NAME_VMA_SHARED_TASKS "vma_shared_tasks_%d"
#define NAME_VMA_SHARED_JOBS "vma_shared_jobs_%d"

enum task_type_t {
	TASK_TYPE_SOMETHING
};

enum event_from_t {
	EVENT_FROM_EFD, // eventfd
	EVENT_FROM_SFD, // signalfd
	EVENT_FROM_TFD, // timerfd
	EVENT_FROM_UFD, // usbfd
	EVENT_FROM_MANUAL
};

enum event_type_t {
	EVENT_TYPE_SIGCHLD
};

enum shared_type_t {
	TYPE_SHARED_WORKER,
	TYPE_SHARED_JOB,
	TYPE_SHARED_BOTH
};

typedef enum {
    MODE_PURE_THREAD_POOL,
    MODE_REACTOR_EPOLL
} pool_mode_t;

struct shared_data_t {
    uintptr_t data_offset;
    uintptr_t list[MAX_LOCAL_QUEUES];
};

struct shared_task_data_t {
    const int id;
    shared_data_t shared;
};

struct shared_job_data_t {
    const int shmid;
    const char *shmname;
    shared_data_t shared;
};

struct pool_t {
	pool_mode_t pooling_mode;
	int epollfd;
	const int id;
	int worker_count;
	int job_count;
	int shared_job_data_count;
	int shared_task_data_count;
	worker_t **workers;
	job_t **jobs;
	sem_t ack_sem;
    vma_zone_t *shared_job_data;
    vma_zone_t *shared_task_data;
};

struct common_task_t {
	const int id;
	const event_from_t from;
	const event_type_t event_type;
	const task_type_t task_type;
	const int fdev;
	void *const context;
	volatile uint64_t last_active_pulse;
	void *(*execute)(common_task_t *self);
	void *(*on_load)(common_task_t *self);
	void *(*on_exit)(common_task_t *self);
	const void *arg;
};

struct task_t {
	common_task_t task_;
	pool_t *operation_ctx;
	worker_t *my_handler;
};

struct worker_t {
	const int id;
	const pthread_t tid;
	arena_t *local_queues; // task_t list
	sem_t sem;
	bool is_sleeping;
	bool shutdown;
	pool_t *parent_pool;
	shared_task_data_t *(*shared)(common_task_t *self, int shared_id); 
	void (*notify_done)(common_task_t *self);
};

struct job_t {
	const int id;
	const char *shmname;
	const event_from_t from;
	const event_type_t event_type;
	const shared_job_data_t *ptr;
	const pid_t pid;
	const void *table_callbacks;
	void *arg;

	void *(*on_exit)(int exit_code);
};

struct job_driver_t {
	const char *name;
	int (*spawn)(job_t *job);
	int (*request_stop)(job_t *job);
	int (*kill)(job_t *job, int signal);
	int (*wait)(job_t *job, int *exit_code);
};

static inline shared_task_data_t *shared(common_task_t *self, int shared_id);
static inline void notify_done(common_task_t *self);
void init_shared_task_data(pool_t *pool, const char *shmname);
shared_task_data_t *get_shared_task_slot(pool_t *pool, int shared_id);
void init_shared_job_data(pool_t *pool, const char *shmname);
shared_job_data_t *get_shared_job_slot(pool_t *pool, int shared_id);

pool_t *init_mode_threading(const int worker_count, const int epollfd, const int pshared);
pool_t *init_mode_processing(const int job_count, const int epollfd);
pool_t *init_mode_threaded_processing(const int worker_count, const int job_count, const int epollfd, const int pshared);

void init_worker(pool_t *pool, int idx_worker, int id, int pshared);
void init_job(pool_t *pool, const int idx_job, const int id,
              const event_from_t evfrom, const event_type_t evtype,
              const char *shmname,
              const void *table_callbacks, void *arg, void *(*on_exit)(int));

void destroy_shared_task_data(pool_t *pool);
void destroy_shared_job_data(pool_t *pool);
void destroy_workers(pool_t *pool);
void destroy_job(pool_t *pool);
void destroy_pool(pool_t *pool);


#endif
