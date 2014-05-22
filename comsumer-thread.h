#ifndef __COMSUMER_THREAD_H__
#define __COMSUMER_THREAD_H__

#define SPIN_COUNT_WAIT (100)
#define CONTEXT_BUF_CACHED (1000)

#include <pthread.h>
#include "reader-thread.h"

typedef struct _context_thread_t {
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t  pushed;
    signal_entry_t  * buf;
    int read;
    int used;
} context_thread_t;

extern context_thread_t * context_thread;
extern char g_exitflag_file[256];

void * comsumer_thread(void * data);
int    wait_context_thread();
int    init_context_thread();

#endif
