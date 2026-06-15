#include "status.h"
#include "test/test.h"
#include "reactor.h"
#include <stdio.h>

volatile bool g_stop_required = false;

int main(void) {
    char msg[64];
    debug_meta_t meta = DEBUG_META(asstatus(CAT_MAINFRAME, CND_SUCCESS, CODE_DESTROY), "on all mode", msg);
    
    printf("=== Starting Heavy Destroy/Leak Test ===\n");
    
    for (int i = 0; i < 1000; i++) {
        /* [ Passed Test] Threading */
        pool_t *pool_tread = init_mode_threading(4, -1, THREAD_PROCESS_SHARED_NO);
        if (pool_tread) {
            destroy_pool(pool_tread);
        }

        /* [ Test on Progress ] Processing */
        pool_t *pool_proc = init_mode_processing(4, -1);
        if (pool_proc) {
            destroy_pool(pool_proc);
        }

        pool_t *pool_th_proc = init_mode_threaded_processing(4, 4, -1, THREAD_PROCESS_SHARED);
        if (pool_th_proc) {
            destroy_pool(pool_th_proc);
        }
        
        if (i > -1 && (i % 1) == 0) {
            snprintf(msg, 64, "Succeeded [ %d ]", i);
            dbgmsg(&meta);
        }
    }

    printf("=== Test Passed Cleanly ===\n");
    return 0;
}
