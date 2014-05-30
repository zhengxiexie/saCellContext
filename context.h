#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdint.h>
#include <time.h>

#include "config.h"
#include "reader-thread.h"

#define CONTEXT_ERR_MALLOC 1

// user information in hash table
typedef struct context_content_t_tag
{
    char			imsi[16];
    char			lac_cell[LEN_LACCELL];
	uint8_t			mobile_open_counts;
	uint8_t			mobile_close_counts;
	uint8_t			smo_sms_counts;
	uint8_t			smt_sms_counts;
	uint8_t			calling_call_counts;
	uint8_t			called_call_counts;
    char			last_lac_cell[LEN_LACCELL];
    uint16_t		resident_time;
    time_t			last_event_time;
    time_t			come_time;
    time_t			leave_time;
    time_t			last_open_time;
    time_t			last_close_time;
    struct			context_content_t_tag * next;
    enum_event_type last_event_type;
#ifdef WITH_MSISDN
    char			msisdn[16];
#endif
#ifdef WITH_IMEI
    char			imei[16];
#endif
}context_content_t;

// each segment in context
typedef struct context_seg_t_tag
{
    uint64_t size;
    context_content_t * content;
    pthread_mutex_t mutex_lock;
}context_seg_t;

// the context
typedef struct context_t_tag
{
    uint64_t size;
    uint32_t part;
    context_seg_t contexts[CONTEXT_PART];
}context_t;

extern context_t context;
extern int context_inited;

int update_context(const signal_entry_t * content);
int check_hourly_update(time_t t);
int dump_context(const char * filename);
int restore_context(const char * filename);
int init_context();
int daily_cleanup(time_t t);

#endif
