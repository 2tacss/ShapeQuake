#include "reactor.h"
#include "allocator.h"
#include "status.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/*
*/

static void *working_loop(void *arg) {
    worker_t *self = (worker_t *)arg;
    if (!self) return nullptr;

    // waiting semaphore and take a task out from local_quueue
    // and run with callback
    return nullptr;
}

pool_t *reactor_init(heap_tracker_t *tracker, int worker_count, int job_count) {
    if (worker_count > MAX_POOLS || job_count > MAX_JOBS) {
        return nullptr;
    }

    status_t st = tracking_health(tracker);
    if (get_cnd(st) == CND_FATAL || get_cnd(st) == CND_ABORT) {
        return nullptr;
    }

    pool_t *pool = (pool_t *)heap_alloc(tracker, sizeof(pool_t));
    if (!pool) return nullptr;

	int pshared = THREAD_PROCESS_NO_SHARED;
    
    if (worker_count > 0 && job_count <= 0) {
        pool->workers = (worker_t *)heap_alloc(tracker, sizeof(worker_t) * worker_count);
    } else if (worker_count <= 0 && job_count > 0) {
        pool->jobs = (job_t *)heap_alloc(tracker, sizeof(job_t) * job_count);
    } else if (worker_count > 0 && job_count > 0) {
        pool->workers = (worker_t *)heap_alloc(tracker, sizeof(worker_t) * worker_count);
        pool->jobs    = (job_t *)heap_alloc(tracker, sizeof(job_t) * job_count);
		pshared = THREAD_PROCESS_SHARED;
    }

    if ((worker_count > 0 && !pool->workers) || (job_count > 0 && !pool->jobs)) {
        return nullptr;
    }

    for (int i = 0; i < worker_count; i++) {
        init_worker(tracker, pool, i, i + 1, pshared);
    }

    return pool;
}

void init_worker(heap_tracker_t *tracker, pool_t *pool, int cur, int id, int pshare) {
    if (!pool || !pool->workers) return;
    sem_init(&pool->workers[cur].sem, pshare, 0);

    pthread_t tid;
    if (pthread_create(&tid, nullptr, working_loop, &pool->workers[cur]) != 0) {
        return;
    }

    *(int *)&pool->workers[cur].id = id;
    *(pthread_t *)&pool->workers[cur].tid = tid;
	pool->workers[cur].notify_done = notify_done;
}

int worker_count(common_task_t *self) {
	if (!self) return -1;
	task_t *task = container_of(self, task_t, super);
	return task->pool->worker_count;
}

task_shared_data_t *shared(common_task_t *self, int shared_id) {
	if (!self) return nullptr;
	task_t *task = container_of(self, task_t, super);

	if (0 > shared_id || shared_id > MAX_TASK_SHARED_DATA) return nullptr;
	return task->pool->shared_task_data[shared_id];
}

void notify_done(common_task_t *self) {
	if (!self) {
		// FATAL: require processing exit safely
	}

	task_t *task = container_of(self, task_t, super);
	if (task->super.fdev > -1) {
		uint64_t flag = 1;
		ssize_t s = write(task->super.fdev, &flag, sizeof(uint64_t));
		if (s != sizeof(uint64_t)) {
			// FATAL: require processing exit safely
		}
	}
}

