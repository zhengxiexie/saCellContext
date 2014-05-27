#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <malloc.h>

#include "context.h"
#include "config.h"
#include "log.h"
#include "reader-thread.h"
#include "comsumer-thread.h"

typedef struct _file_info_t
{
    char filename[256];
}file_info_t;

extern		signal_sort_buffer_t * signal_sort_buf;
extern		pthread_attr_t attr;
volatile	int exit_flag = 0;

time_t		signal_current = -1;
char		g_exitflag_file[256];
int			need_daily_cleanup = 1;

static int		push_to_sort_buf(signal_entry_t * se);
static int		push_to_context(signal_sort_buffer_t * ssb);
static char *	read_line(FILE * f, char * line, size_t byte);
static int		write_back_format(const signal_entry_t * c, char * line);
static int		check_exit_flag();
static int		write_back_context();
static int		init_sort_buffer();
static int		time_format(const time_t t, char * line);
static int		parse_line(const char * line, signal_entry_t * se);

static int file_list_comp(const void * f1, const void * f2)
{
    return strcmp(((file_info_t*)f1)->filename, ((file_info_t*)f2)->filename);
}

static int comp_signal_func(const void * se1, const void * se2)
{
    return ((signal_entry_t*)se1)->timestamp - ((signal_entry_t*)se2)->timestamp;
}

void * read_file_thread(void * data)
{
    int  i = 0, num_of_file = 0, max_num_file = 0;

    DIR  * dp;
    pthread_t * cthread;

    struct stat file_stat;
    struct dirent * entry;

    file_info_t * file_list;

	// allocate sort buffer
    init_sort_buffer();

    int * ts = NULL;

    char backup_filename[256];
    sprintf(backup_filename, "%s/" APP_NAME "_exit.bkp", CFG(backup_dir));

    // init context_thread
    init_context_thread();

    // load backup file
    restore_context(backup_filename);

    // start all context thread
    cthread = (pthread_t*)calloc(CFG(context_thread), sizeof(pthread_t));
    ts = (int*)calloc(CFG(context_thread), sizeof(int));
    for (i = 0; i < CFG(context_thread); i++)
   	{
        ts[i] = i;
        pthread_create(&cthread[i], &attr, comsumer_thread, &ts[i]);
    }

    context_inited = 1;

    logmsg(stdout, "Reading thread started.");

    /*logmsg(stdout, "Memory alloc detail:");*/ //malloc_stats();

    while (1)
   	{
		dp = opendir(CFG(read_dir));
        if (!dp)
	   	{
            logmsg(stderr, "Cannot open dir %s, please check. will retry in %d seconds", CFG(read_dir), CFG(sleep_interval));
            goto retry;
        }

        num_of_file  = 0;
        max_num_file = 32;
        file_list = (file_info_t*)calloc(max_num_file, sizeof(file_info_t));
        while ((entry = readdir(dp)) != NULL)
	   	{
            char read_filename[256];
            sprintf(read_filename, "%s/%s", CFG(read_dir), entry->d_name);
            // skip useless file
            if ((strcmp(read_filename, ".") == 0) || (strcmp(read_filename, "..") == 0)
                || (lstat(read_filename, &file_stat) < 0) || (!S_ISREG(file_stat.st_mode))
				|| strstr(read_filename, ".tmp") != NULL)
			{
				 continue;
			}

            // doble the size of file_list when needed
            if (num_of_file == max_num_file)
		   	{
                file_info_t * tmp_list;
                max_num_file *= 2;
                tmp_list  = file_list;
                file_list = (file_info_t*)calloc(max_num_file, sizeof(file_info_t));
                memcpy(file_list, tmp_list, sizeof(file_info_t) * num_of_file);
                free(tmp_list);
            }

            // copy file info
            strcpy(file_list[num_of_file].filename, read_filename);
			logmsg(stdout, "Readfile name[%s].", read_filename);
            num_of_file++;
        }

        // sort input file
        qsort(file_list, num_of_file, sizeof(file_info_t), file_list_comp);

		if (num_of_file)
		{
			logmsg(stdout, "%d file will be processed this time.", num_of_file);
		}

        for (i = 0; i < num_of_file; i++)
	   	{
            char file_line[MAX_LINE];
            char read_filename[256];
            FILE * rfile;
            signal_entry_t se;
            strcpy(read_filename, file_list[i].filename);
            logmsg(stdout, "Processing file %s", read_filename);
            rfile = fopen(read_filename, "r");
            while (read_line(rfile, file_line, 1024) != NULL)
		   	{
				/*logmsg(stdout, "Pass line[%s]", file_line);*/
				if (parse_line(file_line, &se))
				{
					continue;
				}
                push_to_sort_buf(&se);
            }

            // remove read file
            fclose(rfile);
            remove(read_filename);

            // check exit flag
            if (check_exit_flag())
		   	{
                exit_flag = 1;
                break;
            }
        }

        closedir(dp);

        if (check_exit_flag()) exit_flag = 1;

        if (exit_flag)
	   	{
            logmsg(stdout, "Exitflag recived, exiting read thread...");
            // exiting
            exit_flag = 1;
            // wake all consumer thread
            for (i = 0; i < CFG(context_thread); i++)
		   	{
                pthread_cond_signal(&context_thread[i].pushed);
            }
            for (i = 0; i < CFG(context_thread); i++)
		   	{
                pthread_join(cthread[i], NULL);
            }
            write_back_context();
            dump_context(backup_filename);
            remove(g_exitflag_file);
            break;
        }

        // free allocted resource
        free(file_list);
retry:
		logmsg(stdout, "Now sleep [%d] seconds", CFG(sleep_interval));
        sleep(CFG(sleep_interval));
    }
    return NULL;
}

static int push_to_sort_buf(signal_entry_t * se)
{
    time_t min = se->timestamp / 60;
    int slot = min % CFG(sort_min);
    int i  = 0;
    int ps = 0;
    int ret = 0;

    // discard timeout record
    if (min <= signal_current - CFG(sort_min))
   	{
		logmsg(stderr, "Discard record due to latency, imsi: %s, time %d", se->imsi, se->timestamp);
        return 0;
    }

    if (signal_sort_buf[slot].time < min)
   	{
        signal_current = min;

        // need push sort buffer to context thread
        for (i = 0; i < CFG(sort_min) ; i++)
	   	{
            ps = (i + slot + 1) % CFG(sort_min);
            if (signal_sort_buf[ps].time > signal_sort_buf[slot].time) continue;
            if (signal_sort_buf[ps].time == -1) continue;

            // we need to push this sort buffer into context
            /*logmsg(stdout, "Push %d record to context, timestamp: %d", signal_sort_buf[ps].used, signal_sort_buf[ps].time);*/

            // check hourly update when needed
            check_hourly_update(signal_sort_buf[ps].time * 60);

            // daily cleanup?
            if (((signal_sort_buf[ps].time * 60 + CFG(tz_offset)) / 60 - CFG(cleanup_min)) / 60 % 24 == CFG(cleanup_hour))
		   	{
                if (need_daily_cleanup == 1) {
                    logmsg(stdout, "cleanup started");
                    ret = daily_cleanup(signal_sort_buf[ps].time * 60);
                    logmsg(stdout, "Daily Cleanup finished, %d cleanud", ret);
                }
                need_daily_cleanup = 0;
            } else {
                need_daily_cleanup = 1;
            }

            // do the sort.
            qsort(signal_sort_buf[ps].buffer, signal_sort_buf[ps].used, sizeof(signal_entry_t), comp_signal_func);

            // push to context
            push_to_context(&signal_sort_buf[ps]);

            // reset sort buffer
            signal_sort_buf[ps].used = 0;
            signal_sort_buf[ps].time = -1;
        }
    }

    if (signal_sort_buf[slot].time == -1)
   	{
        // new minute
        signal_sort_buf[slot].time = min;
        signal_sort_buf[slot].used = 0;
    }

    if (signal_sort_buf[slot].used == CFG(sort_buffer))
   	{
        logmsg(stderr, "Sort Buffer Full, please check.");
        return 0;
    }

    memcpy(&signal_sort_buf[slot].buffer[signal_sort_buf[slot].used], se, sizeof(signal_entry_t));

    signal_sort_buf[slot].used += 1;

	logmsg( stdout, "push[%s] to signal_sort_buf[%d], used[%d/%d]\n", se->imsi, slot, signal_sort_buf[slot].used, CFG(sort_buffer));
    return 0;
}

static int push_to_context(signal_sort_buffer_t * ssb)
{
    int cidx = 0, pushed  = 0, to_push = 0;
    int num = ssb->used;
    signal_entry_t * ses = ssb->buffer;
    context_thread_t * ct;

    while (pushed < num)
   	{
		// get num of context to push
		ct = &context_thread[cidx];
#define _DO_PUSH(st, pnum) do { \
    to_push = MIN(num - pushed, pnum); \
    if (to_push == 0) break; \
    memcpy(&ct->buf[st], &ses[pushed], sizeof(signal_entry_t) * to_push); \
	int i = 0;\
	for(; i<to_push; i++ ) \
	{\
		logmsg( stdout, "push[%s] to context_thread[%d]'s buf[%d]\n", (ses+i)->imsi, cidx, st);\
	}\
    pushed   += to_push; \
    ct->used += to_push; \
} while(0)
        pthread_mutex_lock(&ct->mutex);
        if (ct->read + ct->used < CONTEXT_BUF_CACHED)
		{
            _DO_PUSH(ct->read + ct->used, CONTEXT_BUF_CACHED - ct->read - ct->used);
            _DO_PUSH(0, ct->read);
        } else
		{
            _DO_PUSH(ct->read + ct->used - CONTEXT_BUF_CACHED, CONTEXT_BUF_CACHED - ct->used);
        }
#undef _DO_PUSH
        pthread_cond_signal(&ct->pushed);
        pthread_mutex_unlock(&ct->mutex);
        cidx = (cidx + 1) % CFG(context_thread);
    }

    return 0;
}

static char * read_line(FILE * f, char * line, size_t byte)
{
    char * ret = fgets(line, byte, f);
    int  i = 0;
    if (!ret) return NULL;

    // trim EOL
    for (i = strlen(ret) - 1; i >= 0; i--) {
        if (ret[i] == '\r' || ret[i] == '\n') ret[i] = 0;
    }
    return ret;
}

static int check_exit_flag()
{
    FILE * exit_f;
    exit_f = fopen(g_exitflag_file, "r");
	if (exit_f){
		fclose(exit_f);
	}
    return exit_f? 1: 0;
}

static int init_sort_buffer()
{
    int flag = 0;
    int i = 0;

    logmsg(stdout, "Starting to allocate sort buffer");
    signal_sort_buf = calloc(CFG(sort_min), sizeof(signal_sort_buffer_t));
    flag = 0;
    for (i = 0; i < CFG(sort_min); i++)
   	{
        signal_sort_buf[i].size   = CFG(sort_buffer);
        signal_sort_buf[i].time   = -1;
        signal_sort_buf[i].used   = 0;
        signal_sort_buf[i].buffer = calloc(CFG(sort_buffer), sizeof(signal_entry_t));
        if (!signal_sort_buf[i].buffer) {
            flag = 1;
            break;
        }
    }

    if (flag) {
        logmsg(stderr, "Cannot alloc memory for Sort buffer, exiting...");
        exit(2);
    }

    logmsg(stdout, "%.3fMB x %d allocated for sort buffer",
           CFG(sort_buffer) * sizeof(signal_entry_t) / 1024.0 / 1024, CFG(sort_min));
    return 0;
}

// write buffer back to
static int write_back_context()
{
    int i = 0, j = 1, l = 0;
    signal_entry_t * se = NULL;
    FILE * wfile = NULL;
    char line[256];
    char filename[256];

    for (i = 0; i < CFG(sort_min); i++)
   	{
        if (signal_sort_buf[i].time == -1) continue;
        // filename
        snprintf(filename, 255, "%s/%s_%d",
            CFG(read_dir), "00_wb", (int)signal_sort_buf[i].time);
        logmsg(stdout, "Write back contect to file %s", filename);
        wfile = fopen(filename, "w");
        for (j = 0; j < signal_sort_buf[i].used; j++) {
            se = &signal_sort_buf[i].buffer[j];
            l = write_back_format(se, line);
            fwrite(line, l, 1, wfile);
        }
        fflush(wfile);
        fclose(wfile);
    }
    return 0;
}

static int write_back_format(const signal_entry_t * c, char * line)
{
    char tmp_time[27];
    int ret = 0;
    char lac_cell[12];
    char * cell_off = lac_cell;

    memset(tmp_time, 0, 27);
    time_format(c->timestamp, tmp_time);

    strncpy(lac_cell, c->lac_cell, 12);
    cell_off = strchr(lac_cell, '-');
    *(cell_off++) = '\0';

    ret = sprintf(line,
        "%s,%s,%3hu,%s,%s"
#ifdef WITH_MSISDN
        ",%s"
#endif
#ifdef WITH_IMEI
        ",%s"
#endif
        "\n",
        c->imsi, tmp_time, c->event, lac_cell, cell_off
#ifdef WITH_MSISDN
        , c->msisdn
#endif
#ifdef WITH_IMEI
        , c->imei
#endif
    );

    return ret;
}

static int time_format(const time_t t, char * line)
{
    struct tm tp;
    if (t > 0) {
        localtime_r(&t, &tp);
        strftime(line, 27, "%Y-%m-%d %H:%M:%S", &tp);
    } else {
        line[0] = '\0';
    }
    return 0;
}

static int parse_line(const char * line, signal_entry_t * se)
{
    int		  pos = 0;
    char   *  word[15];
    char	  line_tmp[LEN_LINE];
	char   *  word_next = line_tmp;
    int		  w_idx = 1;
    struct tm tmp_tm;

    strcpy(line_tmp, line);
    while (line[pos])
   	{
        if (line[pos] == ',')
	   	{
            line_tmp[pos] = '\0';

            word[w_idx] = word_next;
			w_idx++;
			word_next = line_tmp + pos + 1;

			/*int t = w_idx - 1;*/
			/*logmsg(stderr, "word[%d]:%s\n", t, word[t]);*/
			pos++;
        }

        line_tmp[pos] = line[pos];
        pos++;
    }

    // ckeck num of field
    if (w_idx < num_of_field)
   	{
        logmsg(stderr, "Wrong num of field, need %d, got %d, line: %s", num_of_field, w_idx, line);
        return 1;
    }

    // update se
    strcpy(se->imsi, word[DECODE(imsi)]);
    sprintf(se->lac_cell, "%s-%s", word[DECODE(lac)], word[DECODE(cell)]);
    strptime(word[DECODE(timestamp)], "%Y-%m-%d %H:%M:%S", &tmp_tm);
    se->timestamp = mktime(&tmp_tm);
    se->event = atoi(word[DECODE(event_type)]);
#ifdef WITH_MSISDN
    strcpy(se->msisdn, word[DECODE(msisdn)]);
#endif
#ifdef WITH_IMEI
    strcpy(se->imei, word[DECODE(imei)]);
#endif
    return 0;
}
