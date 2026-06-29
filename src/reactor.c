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

static void *start_working(void *worker) {
	if (!worker) return nullptr;
	worker_t *self = worker;
	if (!self || self->id < MIN_WORKER_ID) {
		if (self && self->parent_pool) {
			debug_meta_t d = DEBUG_META(asstatus(CAT_REACTOR, CND_FATAL, CODE_PARAM), "sem_post()", "Throw");
			dbgmsg(&d);
			sem_post(&self->parent_pool->ack_sem);
		}
		return nullptr;
	}

	while (1) {
		if (self->shutdown) {
			break;
		}
		self->is_sleeping = true;
		sem_wait(&self->sem);
		self->is_sleeping = false;
		if (self->shutdown) {
			break;
		}


		// Handling Queues
		arena_block_t *cur_block = self->local_queues->head;
		size_t read_offset = 0;

		while (cur_block != nullptr) {
			if (cur_block->is_wiped) {
				cur_block = cur_block->next;
				read_offset = 0;
				continue;
			}

			while (read_offset < cur_block->offset) {
				task_t *task = (task_t *)&cur_block->data[read_offset];
				if (!task) {
					debug_meta_t d = DEBUG_META(asstatus(CAT_REACTOR, CND_INFO, CODE_PARAM), "Empty block of arena.", "Something wrong");
					dbgmsg(&d);
				}

				if (task->task_.on_load) {
					task->task_.on_load(&task->task_);
				}

				if (task->task_.execute) {
					task->task_.execute(&task->task_);
				}

				if (self->shared) {
					int id_common_task = self->args_working.id_common_task;
					//self->shared(&task->task_, id_common_task);
				}

				if (task->task_.on_exit) {
					task->task_.on_exit(&task->task_);
				}

				if (self->notify_done) {
					// self->notify_done(&task->task_);
				}

				if ((!self->local_queues->head) && !self->local_queues->is_reset) {
					self->is_done_queues = true;
				}
				read_offset += align(sizeof(task_t));
			}
			cur_block = cur_block->next;
			read_offset = 0;
		}
		arena_reset(self->local_queues);
	}
	self->is_sleeping = true;
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
 *  CALLBACKS `PROCESS DRIVER`                                                *
 * ========================================================================== */
static int process_spawn(job_t *job) {

	pid_t pid = fork();
	if (pid == -1) {
		perror("spawn filed: fork()");
		_exit(127);
	}

	if (pid == 0) {
		//for (int p = 0; p <= 0; p++) {
		while (1) {
            pause(); 
        }
		//}
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
 *  IS FUNCTION                                                               *
 * ========================================================================== */
bool is_evfrom(event_from_t evfrom) {
	if (evfrom < EVENT_FROM_BEGIN || evfrom > EVENT_FROM_END) {
		return false;
	}
	return true;
}

bool is_evtype(event_type_t evtype) {
	if (evtype < EVENT_TYPE_BEGIN || evtype > EVENT_TYPE_END) {
		return false;
	}
	return true;
}

bool is_tktype(task_type_t tktype) {
	if (tktype < TASK_TYPE_BEGIN || tktype > TASK_TYPE_END) {
		return false;
	}
	return true;
}

bool is_pool_mode(pool_mode_t pool_mode) {
	if (pool_mode < POOL_MODE_BEGIN || pool_mode > POOL_MODE_END) {
		return false;
	}
	return true;
}


/* ========================================================================== *
 *  SHARED MEMORY `THREADING` [ VMA ]                                         *
 * ========================================================================== */
void init_shared_task_data(pool_t *pool, const char *shmname, int pshared) {
    if (!pool || !shmname) return;

	char shm_name[MAX_SHM_NAME] = {0};
	strncpy(shm_name, shmname, MAX_SHM_NAME - 1);

    size_t total_size = sizeof(shared_task_data_t) * MAX_SHARED_TASK_DATA;
    pool->shared_task_data = vma_create(shm_name, total_size, pshared);
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

void set_shared_task_data(pool_t *pool, int shared_id, shared_task_data_t shared) {
	if (!pool || !pool->shared_task_data || !pool->shared_task_data->base_addr) return;
	if (SHARED_ID_START_AT > shared_id || shared_id > MAX_SHARED_TASK_DATA) return;
	pool->shared_task_data_count++;
	
	shared_task_data_t *sd = (shared_task_data_t *)vma_alloc(pool->shared_task_data, sizeof(shared_task_data_t));
	memcpy(sd, &shared, sizeof(shared_task_data_t));
}

/* ========================================================================== *
 *  SHARED MEMORY `PROCESSING` [ VMA ]                                        *
 * ========================================================================== */
void init_shared_job_data(pool_t *pool, const char *shmname, int pshared) {
	if (!pool || !shmname) return;

	char shm_name[MAX_SHM_NAME] = {0};
	strncpy(shm_name, shmname, MAX_SHM_NAME - 1);

	size_t total_size = sizeof(shared_job_data_t) * MAX_SHARED_JOB_DATA;

	pool->shared_job_data = vma_create(shm_name, total_size, pshared);
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

void set_shared_job_data(pool_t *pool, int shared_id, shared_job_data_t shared) {
	if (!pool || !pool->shared_job_data || !pool->shared_job_data->base_addr) return;
	pool->shared_job_data_count++;
	
	shared_job_data_t *sd = (shared_job_data_t *)vma_alloc(pool->shared_job_data, sizeof(shared_job_data_t));
	memcpy(sd, &shared, sizeof(shared_job_data_t));
}

/* ========================================================================== *
 *  MODE SWITCHING  [ HEAP TRACKER ]                                           *
 * ========================================================================== */
pool_t *init_mode_threading(const int worker_count, const int epollfd, const int pshared) {
    if (1 > worker_count || worker_count > MAX_POOLS) {
		debug_meta_t meta = DEBUG_META(asstatus(CAT_REACTOR, CND_ABORT, CODE_RANGE),
			"worker_count", "out of range");
			dbgmsg(&meta);
		return nullptr;
	}

	if (THREAD_PROCESS_SHARED_NO > pshared || pshared > THREAD_PROCESS_SHARED) {
		debug_meta_t meta = DEBUG_META(
						asstatus(CAT_REACTOR, CND_ABORT, CODE_RANGE),
						"pshared",
						"out of range range"
						);
		dbgmsg(&meta);
		return nullptr;
	}

    status_t st = tracking_health(&s_tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) {
		debug_meta_t meta = DEBUG_META(asstatus(CAT_REACTOR, CND_ABORT, CODE_ALLOC), "tracking_health", "s_tracker ptr");
		dbgmsg(&meta);
		return nullptr;
	}

	// Zero Cleared
    pool_t *pool = (pool_t *)heap_alloc(&s_tracker, sizeof(pool_t));
    if (!pool) {
		debug_meta_t meta = DEBUG_META(asstatus(CAT_REACTOR, CND_ABORT, CODE_ALLOC),
												"heap_aslloc()", "out of memory or unable to allocate");
		dbgmsg(&meta);
		return nullptr;
	}
	sem_init(&pool->ack_sem, 0, 0);
	
	if (epollfd != -1) {
		pool->pooling_mode = POOL_MODE_REACTOR_EPOLL;
		pool->epollfd = epollfd;
	} else {
		pool->pooling_mode = POOL_MODE_PURE_THREAD_POOL;
		pool->epollfd = -1;
	}

	pool->workers = (worker_t **)heap_alloc(&s_tracker, sizeof(worker_t *) * worker_count);
	if (!pool->workers) {
		heap_free(&s_tracker, pool);
		sem_destroy(&pool->ack_sem);

		debug_meta_t meta = DEBUG_META(asstatus(CAT_REACTOR, CND_ABORT, CODE_ALLOC),
											"heap_aslloc()", "out of memory or unable to allocate");
		dbgmsg(&meta);
		return nullptr;
	}

	pool->worker_count = worker_count;
	for (int i = 0; i < worker_count; i++) {
		init_worker(pool, i, i + 1, pshared);
	}
    return pool;
}

pool_t *init_mode_processing(const int job_count, const int epollfd) {
    if (1 > job_count || job_count > MAX_JOBS) return nullptr;

    status_t st = tracking_health(&s_tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
	pool_t *pool = (pool_t *)heap_alloc(&s_tracker, sizeof(pool_t));
    if (!pool) return nullptr;
	if (epollfd != -1) {
		pool->pooling_mode = POOL_MODE_REACTOR_EPOLL;
		pool->epollfd = epollfd;
	} else {
		pool->pooling_mode = POOL_MODE_PURE_THREAD_POOL;
		pool->epollfd = -1;
	}

	sem_init(&pool->ack_sem, 0, 0);

    pool->jobs = (job_t **)heap_alloc(&s_tracker, sizeof(job_t *) * job_count);
    if (!pool->jobs) {
		heap_free(&s_tracker, pool);
		sem_destroy(&pool->ack_sem);
		return nullptr;
	}
    
    pool->job_count = job_count;
    for (int i = 0; i < job_count; i++) {
        init_job(pool, i, i + 1, 0, 0, nullptr, nullptr, nullptr, nullptr);
    }

    *(int *)&pool->id = 0;
    return pool;
}

pool_t *init_mode_threaded_processing(const int worker_count, const int job_count, const int epollfd, const int pshared) {
    if ((1 > worker_count || worker_count > MAX_POOLS) ||
		(1 > job_count || job_count > MAX_JOBS))
	{
        return nullptr;
    }

	if (THREAD_PROCESS_SHARED_NO > pshared || pshared > THREAD_PROCESS_SHARED) {
        debug_meta_t meta = DEBUG_META(asstatus(CAT_REACTOR, CND_ABORT, CODE_RANGE), "pshared", "out of range range");
        dbgmsg(&meta);
        return nullptr;
    }

    status_t st = tracking_health(&s_tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
    pool_t *pool = (pool_t *)heap_alloc(&s_tracker, sizeof(pool_t));
    if (!pool) return nullptr;

	if (epollfd != -1) {
		pool->pooling_mode = POOL_MODE_REACTOR_EPOLL;
		pool->epollfd = epollfd;
	} else {
		pool->pooling_mode = POOL_MODE_PURE_THREAD_POOL;
		pool->epollfd = -1;
	}
	sem_init(&pool->ack_sem, 0, 0);
	
    pool->jobs = (job_t **)heap_alloc(&s_tracker, sizeof(job_t *) * job_count);
	if (!pool->jobs) {
		heap_free(&s_tracker, pool);
		return nullptr;
	}
	
    pool->workers = (worker_t **)heap_alloc(&s_tracker, sizeof(worker_t *) * worker_count);
	if (!pool->workers) {
		heap_free(&s_tracker, pool->jobs);
		heap_free(&s_tracker, pool);
		return nullptr;
	}

	pool->job_count = job_count;
	pool->worker_count = worker_count;
    for (int i = 0; i < job_count; i++) init_job(pool, i, i + 1, 0, 0, nullptr, nullptr, nullptr, nullptr);
    for (int i = 0; i < worker_count; i++) init_worker(pool, i, i + 1, pshared);

	*(int *)&pool->id = 0;
    return pool;
}

/* ========================================================================== *
 *  THREADING [ HEAP TRACKER] [ ARENA ]                                       *
 * ========================================================================== */
void init_worker(pool_t *pool, int idx_worker, int id, int pshared) {
    status_t st = tracking_health(&s_tracker);
    if (CND_FATAL == get_cnd(st) || get_cnd(st) == CND_ABORT) return;

    if (!pool || !pool->workers) return;
    if (0 > idx_worker || idx_worker >= pool->worker_count ||
        id < 0 || (THREAD_PROCESS_SHARED_NO > pshared || pshared > THREAD_PROCESS_SHARED)) return;

    worker_t *w = (worker_t *)heap_alloc(&s_tracker, sizeof(worker_t));
    if (!w) return;
    pool->workers[idx_worker] = w;

    const worker_t temp = { .id = id, .shared = shared, .notify_done = notify_done };
    memcpy(w, &temp, sizeof(worker_t));

	w->local_queues = arena_init(SIZE_ARENA_DEFAULT);
    sem_init(&w->sem, pshared, 0);
	w->parent_pool = pool;
	args_working_t args_working = {
		.id_common_task = idx_worker,
	};
	w->args_working = args_working;

    pthread_t tid;
    if (pthread_create(&tid, nullptr, start_working, w) == 0) {
        memcpy((void *)&w->tid, &tid, sizeof(pthread_t));
    } else {
		debug_meta_t d = DEBUG_META(
			asstatus(CAT_REACTOR, CND_FATAL, CODE_THREAD_CREATE),
			"pthread_create()",
			"&tid, nullptr, start_working, &w"
		);
		dbgmsg(&d);
	}
}

/* ========================================================================== *
 *  PROCESSING [HEAP TRACKER]                                                 *
 * ========================================================================== */
void init_job(pool_t *pool, const int idx_job, const int id,
              const event_from_t evfrom, const event_type_t evtype,
              const char *shmname,
              const void *table_callbacks, void *arg, void *(*on_exit)(int)) {
    status_t st = tracking_health(&s_tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return;
    
    if (!pool || !pool->jobs || idx_job < 0 || idx_job >= pool->job_count || id < 0) return;
    
	job_t *j = (job_t *)heap_alloc(&s_tracker, sizeof(job_t));
    if (!j) return;
    pool->jobs[idx_job] = j;

    const job_t temp = {
        .id = id,
        .shmname = shmname,
        .evfrom = evfrom,
        .evtype = evtype,
		.pid = -1,
        .table_callbacks = table_callbacks,
        .arg = arg,
        .on_exit = on_exit
    };

    memcpy(pool->jobs[idx_job], &temp, sizeof(job_t));
	if (process_driver.spawn(j) != 0) {
        // FATAL: Failed to spawn job
    }
}

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
	if (!pool || pool->pooling_mode == POOL_MODE_PURE_THREAD_POOL) return;
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
		if (!pool->workers[i]) continue;
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
	if (!pool || pool->pooling_mode == POOL_MODE_PURE_THREAD_POOL) return;

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
	if (POOL_MODE_REACTOR_EPOLL == pool->pooling_mode) {
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
