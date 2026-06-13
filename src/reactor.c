#include "reactor.h"
#include "allocator.h"
#include "status.h"
#include "vma.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

/*
*/

static void *working_loop(void *arg) {
    worker_t *self = (worker_t *)arg;
    if (!self) return nullptr;

    // waiting semaphore and take a task out from local_quueue
    // and run with callback
    return nullptr;
}

static inline void *on_exit(int something) {
	return nullptr;
}


/* ========================================================================== *
 *  CALLBACKS `THREADING`                                                     *
 * ========================================================================== */
static shared_task_data_t *shared(common_task_t *self, int shared_id) {
	if (!self) return nullptr;
	task_t *task = container_of(self, task_t, task_);

	if (0 > shared_id || shared_id > MAX_SHARED_TASK_DATA) return nullptr;
	return task->operation_ctx->shared_task_data[shared_id];
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
/*
static int process_spawn(job_t *job) {
	snprintf(*(const char **)&job->shmname, sizeof(job->shmname), "/sq_shm_%d", idx_job);

	int fd_shm = shm_open(job->shmname, O_CREAT | O_RDWR, 0666);
	if (fd_shm == -1) {
		return -1;
	}

	ftruncate(fd_shm, sizeof(shared_job_data_t));

	job->shm = (shared_job_data_t *)mmap(
		nullptr, sizeof(shared_job_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0
	);
	close(fd_shm);

	job->shm->progress = 0;
	snprintf(job->shm->message, sizeof(job->shm->message), "Initializing.");

	job->ev_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (job->ev_fd == -1) {
		return -1;
	}

	pid_t pid = fork();
	if (pid == -1) return -1;

	if (pid == 0) {
		for (int p = 0; p <= 0; p++) {
			sleep(1);

			job->shm->progress = p * 33;
			if (p == 3) {
				job->shm->progress = 100;
			}
			snprintf(job->shm->message, sizeof(job->shm->message), "Running step: %d", p);

			uint64_t pulse = 1;
			write(job->ev_fd, &pulse, sizeof(pulse));

			munmap(job->shm, sizeof(shared_job_data_t));
			_exit(0);
		}
	}

	job->handle.pid = pid;
	job->type = JOB_TYPE_PROCESS;
	return 0;
}

static int process_spawn1(const char *cmd, job_t *job) {
	pid_t pid = fork();
	if (pid == 0) {
		execl("/bin/sh", "sh", "-c", cmd, NULL);
		_exit(1);
	} else if (pid > 0) {
		job->handle.pid = pid;
		job->type = JOB_TYPE_PROCESS;
		return 0;
	}
	return -1;
}

static int process_kill(job_t *job, int signal) {
	if (!job || job->type != JOB_TYPE_PROCESS) {
		return -1;
	}
	return kill(job->handle.pid, signal);
}

static int process_request_stop(job_t *job) {
	return kill(job->handle.pid, SIGTERM);
}

static int process_wait(job_t job, int *exit_code) {
	return waitpid(job.handle.pid, exit_code, 0);
}

const job_driver_t process_driver = {
	.name = "process_driver",
	.spawn = process_spawn,
	.request_stop = process_request_stop,
	.kill = process_kill,
	.wait = process_wait
};

*/

/* ========================================================================== *
 *  SHARED MEMORY `THREADING`                                                 *
 * ========================================================================== */

void init_shared_task_data(pool_t *pool) {
    if (!pool) return;

    size_t total_size = sizeof(shared_task_data_t) * MAX_SHARED_TASK_DATA;

    pool->shared_task_data = vma_create(NAME_VMA_SHARED_TASKS, total_size);
    if (!pool->shared_task_data) return;

    pool->shared_task_data_count = MAX_SHARED_TASK_DATA;
}

shared_task_data_t *get_shared_task_slot(pool_t *pool, int shared_id) {
    if (!pool || !pool->shared_task_data) return nullptr;
    if (0 > shared_id || shared_id >= MAX_SHARED_TASK_DATA) return nullptr;

    shared_task_data_t *array = (shared_task_data_t *)pool->shared_task_data->base_addr;
    return &array[shared_id];
}

/* ========================================================================== *
 *  SHARED MEMORY `PROCESSING`                                                *
 * ========================================================================== */
void init_shared_job_data(pool_t *pool) {
	if (!pool) return;

	size_t total_size = sizeof(shared_job_data_t) * MAX_SHARED_JOB_DATA;

	pool->shared_job_data = vma_create(NAME_VMA_SHARED_JOBS, total_size);
	if (!pool->shared_job_data) return;

	pool->shared_job_data_count = MAX_SHARED_JOB_DATA;
	return;
}

shared_job_data_t *get_shared_job_slot(pool_t *pool, int shared_id) {
    if (!pool || !pool->shared_task_data) return nullptr;
    if (0 > shared_id || shared_id >= MAX_SHARED_JOB_DATA) return nullptr;

    shared_job_data_t *array = (shared_job_data_t *)pool->shared_job_data->base_addr;
    return &array[shared_id];
}

/* ========================================================================== *
 *  MODE SWITCHING                                                            *
 * ========================================================================== */
pool_t *init_mode_threading(heap_tracker_t *tracker, int worker_count){
    if (0 > worker_count || worker_count > MAX_POOLS) return nullptr;

    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
    pool_t *pool = (pool_t *)heap_alloc(tracker, sizeof(pool_t));
    if (!pool) return nullptr;

	int pshared = THREAD_PROCESS_NO_SHARED;

    pool->workers = (worker_t *)heap_alloc(tracker, sizeof(worker_t) * worker_count);
	if (!pool->workers) return nullptr;
	pool->worker_count = worker_count;
    for (int i = 0; i < worker_count; i++) init_worker(tracker, pool, i, i + 1, pshared);

	*(int *)&pool->id = 0;

    return pool;
}

pool_t *init_mode_processing(heap_tracker_t *tracker, const int job_count) {    
    if (0 > job_count || job_count > MAX_JOBS) return nullptr;

    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
    pool_t *pool = (pool_t *)heap_alloc(tracker, sizeof(pool_t));
    if (!pool) return nullptr;

	pool->jobs = (job_t *)heap_alloc(tracker, sizeof(job_t) * job_count);
	if (!pool->jobs) return nullptr;
	pool->job_count = job_count;
    for (int i = 0; i < job_count; i++) {
		init_job(tracker, pool, i, i + 1,
			0, 0, nullptr, nullptr, nullptr, nullptr, nullptr);
	}

	*(int *)&pool->id = 0;

    return pool;
}

pool_t *init_mode_threaded_processing(heap_tracker_t *tracker, int worker_count, int job_count) {
    if (( 0 > worker_count || worker_count > MAX_POOLS) ||
			(0 > job_count || job_count > MAX_JOBS)) return nullptr;

    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return nullptr;

	// Zero Cleared
    pool_t *pool = (pool_t *)heap_alloc(tracker, sizeof(pool_t));
    if (!pool) return nullptr;

	int pshared = THREAD_PROCESS_SHARED;
    pool->jobs    = (job_t *)heap_alloc(tracker, sizeof(job_t) * job_count);
	if (!pool->jobs) return nullptr;
    pool->workers = (worker_t *)heap_alloc(tracker, sizeof(worker_t) * worker_count);
	if (!pool->workers) return nullptr;

	pshared = THREAD_PROCESS_SHARED;
	pool->job_count = job_count;
	pool->worker_count = worker_count;
    for (int i = 0; i < job_count; i++) {
		init_job(tracker, pool, i, i + 1,
			0, 0, nullptr, nullptr, nullptr, nullptr, nullptr);
		}
    for (int i = 0; i < worker_count; i++) init_worker(tracker, pool, i, i + 1, pshared);

	*(int *)&pool->id = 0;

    return pool;
}

/* ========================================================================== *
 *  THREADING                                                                 *
 * ========================================================================== */
status_t init_worker(heap_tracker_t *tracker, pool_t *pool, int idx_worker, int id, int pshare) {
	status_t st = tracking_health(tracker);
	if (CND_FATAL == get_cnd(st) || get_cnd(st) == CND_ABORT) return st;

    if (!pool || !pool->workers) return;
	if (0 > idx_worker || idx_worker > pool->worker_count ||
				id < 0 || (0 > pshare || pshare > THREAD_PROCESS_SHARED )) return;
	
    sem_init(&pool->workers[idx_worker].sem, pshare, 0);

    pthread_t tid;
    if (pthread_create(&tid, nullptr, working_loop, &pool->workers[idx_worker]) != 0) {
        return;
    }

    *(int *)&pool->workers[idx_worker].id = id;
    *(pthread_t *)&pool->workers[idx_worker].tid = tid;
	pool->workers[idx_worker].shared = shared;
	pool->workers[idx_worker].notify_done = notify_done;
}


/* ========================================================================== *
 *  PROCESSING                                                                *
 * ========================================================================== */
void init_job(heap_tracker_t *tracker, pool_t *pool, const int idx_job, const int id,
              const event_from_t evfrom, const event_type_t evtype,
              const char *shmname, const shared_job_data_t *shm,
              const void *table_callbacks, void *arg, void *(*on_exit)(int)) {
    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) return;
    
    if (!pool || !pool->jobs || idx_job < 0 || idx_job >= pool->job_count || id < 0) return;
    
    *(int *)&pool->jobs[idx_job].id = id;
    *(const char **)&pool->jobs[idx_job].shmname = shmname;
    *(event_from_t *)&pool->jobs[idx_job].from = evfrom;
    *(event_type_t *)&pool->jobs[idx_job].event_type = evtype;
    *(const job_shared_data_t **)&pool->jobs[idx_job].shm = shm;
    
    pool->jobs[idx_job].table_callbacks = table_callbacks;
    pool->jobs[idx_job].arg = arg;
    pool->jobs[idx_job].on_exit = on_exit;
}

