#include "reactor.h"
#include "allocator.h"
#include "heap.h"
#include "status.h"
#include "vma.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "test/test.h"

static heap_tracker_t s_tracker = {0};

static void *working_loop(void *arg) {
    worker_t *self = (worker_t *)arg;
    if (!self) return nullptr;

	while (1) {
		self->is_sleeping = true;
		sem_wait(&self->sem);
		// do smething
		self->is_sleeping = false;
		
		if (self->shutdown) {
			break;
		}
		
	    // waiting semaphore and take a task out from local_quueue
	    // and run with callback
	}
	sem_post(&self->parent_pool->ack_sem);
    return nullptr;
}

static inline void *on_exit(int something) {
	(void)something;
	return nullptr;
}


/* ========================================================================== *
 *  CALLBACKS `THREADING`                                                     *
 * ========================================================================== */
static shared_task_data_t *shared(common_task_t *self, int shared_id) {
    if (!self) return nullptr;
    task_t *task = container_of(self, task_t, task_);

    if (SHARED_ID_START_AT > shared_id || shared_id > MAX_SHARED_TASK_DATA) return nullptr;
    
    return get_shared_task_slot(task->operation_ctx, shared_id);
}

static void notify_done(common_task_t *self) {
	if (!self) {
		// FATAL: require processing exit safely
	}

	task_t *task = container_of(self, task_t, task_);
	if (task->task_.fdev > -1) {
		uint64_t flag = 1;
		ssize_t s = write(task->task_.fdev, &flag, sizeof(uint64_t));
		if (s != sizeof(uint64_t)) {
			// FATAL: require processing exit safely
		}
	}
}

/* ========================================================================== *
 *  CALLBACKS `JOB DRIVER`                                                    *
 * ========================================================================== */
static int process_spawn(job_t *job) {

	pid_t pid = fork();
	if (pid == -1) {
		perror("spawn filed: fork()");
		_exit(127);
	}

	if (pid == 0) {
		for (int p = 0; p <= 0; p++) {
			_exit(0);
		}
	}

	memcpy((void *)&job->pid, &pid, sizeof(pid_t));
	return 0;
}

static int process_kill(job_t *job, int signal) {
	if (!job || job->pid < 1) return 1;
	return kill(job->pid, signal);
}

static int process_request_stop(job_t *job) {
	if (!job || job->pid < 1) return 1;
	return kill(job->pid, SIGTERM);
}

static int process_wait(job_t *job, int *exit_code) {
    if (!job || job->pid <= 0) return -1;

    int exit_status;
    pid_t res = waitpid(job->pid, &exit_status, 0);
    if (res > 0) {
        if (WIFEXITED(exit_status) && exit_code) {
            *exit_code = WEXITSTATUS(exit_status);
        }
        return 0;
    }
    return -1;
}

const job_driver_t process_driver = {
	.name = "process_driver",
	.spawn = process_spawn,
	.request_stop = process_request_stop,
	.kill = process_kill,
	.wait = process_wait
};


/* ========================================================================== *
 *  SHARED MEMORY `THREADING`                                                 *
 * ========================================================================== */
void init_shared_task_data(pool_t *pool, const char *shmname) {
    if (!pool || !shmname) return;

	char shm_name[MAX_SHM_NAME] = {0};
	strncpy(shm_name, shmname, MAX_SHM_NAME - 1);

    size_t total_size = sizeof(shared_task_data_t) * MAX_SHARED_TASK_DATA;
    pool->shared_task_data = vma_create(shm_name, total_size);
    if (!pool->shared_task_data) return;

    void *array_rack = vma_alloc(pool->shared_task_data, total_size);
    if (!array_rack) return;

    pool->shared_task_data_count = SHARED_ID_START_AT;
}

shared_task_data_t *get_shared_task_slot(pool_t *pool, int shared_id) {
    if (!pool || !pool->shared_task_data || !pool->shared_task_data->base_addr) return nullptr;
    if (SHARED_ID_START_AT > shared_id || shared_id > MAX_SHARED_TASK_DATA) return nullptr;

    shared_task_data_t *array = (shared_task_data_t *)pool->shared_task_data->base_addr;
    return &array[shared_id];
}

/* ========================================================================== *
 *  SHARED MEMORY `PROCESSING`                                                *
 * ========================================================================== */
void init_shared_job_data(pool_t *pool, const char *shmname) {
	if (!pool || !shmname) return;

	char shm_name[MAX_SHM_NAME] = {0};
	strncpy(shm_name, shmname, MAX_SHM_NAME - 1);

	size_t total_size = sizeof(shared_job_data_t) * MAX_SHARED_JOB_DATA;

	pool->shared_job_data = vma_create(shm_name, total_size);
	if (!pool->shared_job_data) return;
	
	void *array_rack = vma_alloc(pool->shared_job_data, total_size);
    if (!array_rack) return;

    pool->shared_job_data_count = SHARED_ID_START_AT;
}

shared_job_data_t *get_shared_job_slot(pool_t *pool, int shared_id) {
    if (!pool || !pool->shared_job_data || !pool->shared_job_data->base_addr) return nullptr;
    if (SHARED_ID_START_AT > shared_id || shared_id > MAX_SHARED_JOB_DATA) return nullptr;

    shared_job_data_t *array = (shared_job_data_t *)pool->shared_job_data->base_addr;
    return &array[shared_id];
}

/* ========================================================================== *
 *  MODE SWITCHING                                                            *
 * ========================================================================== */
pool_t *init_mode_threading(const int worker_count, const int epollfd, const int pshared) {
    if (0 > worker_count || worker_count > MAX_POOLS) return nullptr;

    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
    pool_t *pool = (pool_t *)heap_alloc(tracker, sizeof(pool_t));
    if (!pool) return nullptr;

	int pshared = THREAD_PROCESS_NO_SHARED;

	pool->workers = (worker_t **)heap_alloc(tracker, sizeof(worker_t *) * worker_count);
	if (!pool->workers) return nullptr;

	for (int i = 0; i < worker_count; i++) {
		init_worker(tracker, pool, i, i + 1, pshared);
	}
    return pool;
}

pool_t *init_mode_processing(const int job_count, const int epollfd) {
    if (0 > job_count || job_count > MAX_JOBS) return nullptr;

    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
	pool_t *pool = (pool_t *)heap_alloc(tracker, sizeof(pool_t));
    if (!pool) return nullptr;

    pool->jobs = (job_t **)heap_alloc(tracker, sizeof(job_t *) * job_count);
    if (!pool->jobs) return nullptr;
    
    pool->job_count = job_count;
    for (int i = 0; i < job_count; i++) {
        init_job(tracker, pool, i, i + 1, 0, 0, nullptr, nullptr, nullptr, nullptr);
    }
    *(int *)&pool->id = 0;
    return pool;
}

pool_t *init_mode_threaded_processing(const int worker_count, const int job_count, const int epollfd, const int pshared) {
    if (( 0 > worker_count || worker_count > MAX_POOLS) ||
			(0 > job_count || job_count > MAX_JOBS)) return nullptr;

    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
    pool_t *pool = (pool_t *)heap_alloc(tracker, sizeof(pool_t));
    if (!pool) return nullptr;
	
    pool->jobs = (job_t **)heap_alloc(tracker, sizeof(job_t *) * job_count);
	if (!pool->jobs) return nullptr;
    pool->workers = (worker_t **)heap_alloc(tracker, sizeof(worker_t *) * worker_count);
	if (!pool->workers) return nullptr;

	int pshared = THREAD_PROCESS_SHARED;
	pool->job_count = job_count;
	pool->worker_count = worker_count;
    for (int i = 0; i < job_count; i++) init_job(tracker, pool, i, i + 1, 0, 0, nullptr, nullptr, nullptr, nullptr);
    for (int i = 0; i < worker_count; i++) init_worker(tracker, pool, i, i + 1, pshared);

	*(int *)&pool->id = 0;

    return pool;
}

/* ========================================================================== *
 * THREADING                                                                 *
 * ========================================================================== */
void init_worker(heap_tracker_t *tracker, pool_t *pool, int idx_worker, int id, int pshared) {
    status_t st = tracking_health(tracker);
    if (CND_FATAL == get_cnd(st) || get_cnd(st) == CND_ABORT) return;

    if (!pool || !pool->workers) return;
    if (0 > idx_worker || idx_worker >= pool->worker_count ||
        id < 0 || (THREAD_PROCESS_NO_SHARED > pshared || pshared > THREAD_PROCESS_SHARED)) return;

    worker_t *w = (worker_t *)heap_alloc(tracker, sizeof(worker_t));
    if (!w) return;
    pool->workers[idx_worker] = w;

    const worker_t temp = { .id = id, .shared = shared, .notify_done = notify_done };
    memcpy(w, &temp, sizeof(worker_t));

    sem_init(&w->sem, pshared, 0);

    pthread_t tid;
    if (pthread_create(&tid, nullptr, working_loop, w) == 0) {
        memcpy((void *)&w->tid, &tid, sizeof(pthread_t));
    }
}

/* ========================================================================== *
 * PROCESSING                                                                *
 * ========================================================================== */
void init_job(pool_t *pool, const int idx_job, const int id,
              const event_from_t evfrom, const event_type_t evtype,
              const char *shmname,
              const void *table_callbacks, void *arg, void *(*on_exit)(int)) {
    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return;
    
    if (!pool || !pool->jobs || idx_job < 0 || idx_job >= pool->job_count || id < 0) return;
    
    const job_t temp = {
        .id = id,
        .shmname = shmname,
        .from = evfrom,
        .event_type = evtype,
        .table_callbacks = table_callbacks,
        .arg = arg,
        .on_exit = on_exit
    };

    memcpy(&pool->jobs[idx_job], &temp, sizeof(job_t));
void destroy_shared_task_data(pool_t *pool) {
	if (!pool || !pool->shared_task_data) return;
	if (pool->shared_task_data_count > SHARED_ID_PROTECTED_ZONE) return;

	if (pool->workers == nullptr && pool->worker_count == SHARED_DATA_COUNT_NONE) {
		vma_destroy(pool->shared_task_data);
		pool->shared_task_data = nullptr;
		pool->shared_task_data_count = SHARED_DATA_COUNT_NONE;
	}
}

void destroy_shared_job_data(pool_t *pool) {
	if (!pool) return;
	if (pool->shared_job_data_count > SHARED_ID_PROTECTED_ZONE) return;

	if (!pool->jobs && pool->job_count == SHARED_DATA_COUNT_NONE) {
		vma_destroy(pool->shared_job_data);
		pool->shared_job_data = nullptr;
		pool->shared_job_data_count = SHARED_DATA_COUNT_NONE;
	}
}

void destroy_workers(pool_t *pool) {
	if (!pool) return;

	status_t s = tracking_health(&s_tracker);
	if (CND_ABORT == get_cnd(s) || CND_FATAL == get_cnd(s)) return;

	for (int i = 0; i < pool->worker_count; i++) {
		if (!pool->workers[i]) continue;

	    worker_t *w = pool->workers[i];
	    if (w) {
	        w->shutdown = true;
	        sem_post(&w->sem);
	    }
	}

	for (int i = 0; i < pool->worker_count; i++) {
		sem_wait(&pool->ack_sem);
	}

	for (int i = 0; i < pool->worker_count; i++) {
		if (!pool->workers[i]) continue;

	    worker_t *w = pool->workers[i];
		if (!w->local_queues->is_reset) {
			// Possibly Local Task Remained
			// local queue manager MUST run arena_reset() with the flag `ARENA_REQUEST_RESET_OFFSET`,
			// When all done.
			arena_destroy(w->local_queues, ARENA_FORCE_DESTROY);
		} else {
			arena_destroy(w->local_queues, ARENA_FORCE_DESTROY);
		}

	    if (w && w->tid) {
	        pthread_join(w->tid, nullptr);
	    }

		sem_destroy(&w->sem);
		heap_free(&s_tracker, w);
	}

	heap_free(&s_tracker, pool->workers);
	pool->workers = nullptr;
	pool->worker_count = SHARED_DATA_COUNT_NONE;
}

void destroy_job(pool_t *pool) {
	if (!pool) return;

	status_t s = tracking_health(&s_tracker);
	if (CND_ABORT == get_cnd(s) || CND_FATAL == get_cnd(s)) return;

	char msg[64] = {0};
	bool fire_log = false;

	for (int i = 0; i < pool->job_count; i++) {
		if (pool->jobs[i] && pool->jobs[i]->pid > 0) {
			process_driver.kill(pool->jobs[i], SIGTERM);
		}
	}

	for (int i = 0; i < pool->job_count; i++) {
		if (!pool->jobs[i]) continue;
		
		job_t *j = pool->jobs[i];
		if (j->pid > 0) {
			//int final_code = -1;
			int exit_status;
		    if (waitpid(j->pid, &exit_status, 0) > 0) {
				fire_log = false;
				if (WIFEXITED(exit_status)) {
					int code = WEXITSTATUS(exit_status);
					snprintf(msg, 64, "Exit status with %d", code);
					fire_log = true;
				} else if (WIFSIGNALED(exit_status)) {
					int sig = WTERMSIG(exit_status);
					snprintf(msg, 64, "Exit signal by %d", sig);
					fire_log = true;
				}
				
				if (fire_log) {
					debug_meta_t meta = DEBUG_META(
										asstatus(CAT_REACTOR, CND_INFO, CODE_EXIT),
										"waitpid()",
										msg
									);
					dbgmsg(&meta);
				}
			}
			// if (j->on_exit) j->on_exit(final_code);
		}
		heap_free(&s_tracker, j);
		pool->jobs[i] = nullptr;
		j = nullptr;
	}

	heap_free(&s_tracker, pool->jobs);
	pool->jobs = nullptr;
	pool->job_count = SHARED_DATA_COUNT_NONE;
}

void destroy_pool(pool_t *pool) {
	if (MODE_REACTOR_EPOLL == pool->pooling_mode) {
		if (pool->epollfd != -1) close(pool->epollfd);
	}
	destroy_job(pool);
	destroy_workers(pool);
	destroy_shared_job_data(pool);
	destroy_shared_task_data(pool);
	sem_destroy(&pool->ack_sem);
    *(int *)&pool->id = INVALID_ID;
    heap_free(&s_tracker, pool);
}
