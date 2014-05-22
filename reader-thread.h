#ifndef __READER_THREAD_H__
#define __READER_THREAD_H__

#define MAX_LINE (1024)

#include <pthread.h>
#include <time.h>
#include <stdint.h>

#include "config.h"

extern volatile int exit_flag;

struct signal_entry_t
{
    char  imsi[16];
    time_t timestamp;
    char lac_cell[LEN_LACCELL];
	#ifdef WITH_MSISDN
    char     msisdn[16];
	#endif
	#ifdef WITH_IMEI
    char     imei[16];
	#endif
    enum_event_type event;
};
typedef struct signal_entry_t signal_entry_t;

struct signal_sort_buffer_t
{
    int    size;
    int    used;
    time_t time;
    signal_entry_t * buffer;
};
typedef struct signal_sort_buffer_t signal_sort_buffer_t;

void * read_file_thread(void * data);

#endif

