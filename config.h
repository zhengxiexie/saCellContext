#ifndef __CONFIG_H__
#define __CONFIG_H__

#define DEF_DECODE(x) int field_##x = -1
#define EXPORT_DECODE(x) extern int field_##x
#define DECODE(x) field_##x
#define CFG(x) (g_config._##x)
#define SIZE_OF(x) (sizeof(x) / sizeof(x[0]))

#define DECODE_LINE_LENGTH (256)
#define CONFIG_KEY_VALUE_LENGTH (256)
#define CONFIG_LENGTH (1024)
#define LEN_LACCELL (12)
#define LEN_LINE (1024)
#define CONTEXT_BASESIZE      (1000000ul)
#define CONTEXT_SORT_BASESIZE (100000ul)
#define CONTEXT_PART (10)

#define APP_NAME "saCellContext"

#define MAX(a, b) ((a) > (b)? (a): (b))
#define MIN(a, b) ((a) < (b)? (a): (b))

#define GET_DECODE(x) if (0 == strcasecmp(line, #x)) { \
	DECODE(x) = atoi(comma); \
	logmsg(stdout, "field_%s[%d]", #x, DECODE(x)); \
	num_of_field ++; \
	continue; \
}

#define GET_STR_CFG(x) if (0 == strcasecmp(word[0], #x)) { \
	strcpy(g_config._##x, word[1]); \
	logmsg(stdout, "g_config._%s[%s]", #x, word[1]); \
	continue; \
}

#define GET_INT_CFG(x) if (0 == strcasecmp(word[0], #x)) { \
	g_config._##x = atoi(word[1]); \
	logmsg(stdout, "g_config._%s[%d]", #x, g_config._##x); \
	continue; \
}

#define GET_MAP_INT_CFG(x) if (0 == strcasecmp(word[0], #x)) { \
	int __x = atoi(word[1]); \
	__x = __x > SIZE_OF(cfgmap_##x)? SIZE_OF(cfgmap_##x): __x; \
	__x = __x < 0? 0: __x; \
	g_config._##x = cfgmap_##x[__x]; \
	logmsg(stdout, "g_config._%s[%d]", #x, g_config._##x); \
	continue; \
}

EXPORT_DECODE(imsi);
EXPORT_DECODE(timestamp);
EXPORT_DECODE(event_type);
EXPORT_DECODE(lac);
EXPORT_DECODE(cell);
EXPORT_DECODE(msisdn);
EXPORT_DECODE(imei);

extern int num_of_field;

typedef struct config_t_tag
{
    int  _sort_min;
    int  _sort_buffer;
    int  _output_interval;
    int  _context_size;
    int  _context_thread;
    int  _sleep_interval;
    int  _cleanup_mark;
    int  _cleanup_min;
    int  _cleanup_hour;
    int  _cross_mountpoint;
    time_t _tz_offset;
    char _output_dir[128];
    char _output_prefix[64];
    char _output_suffix[64];
    char _read_dir[128];
    char _backup_dir[256];
    char _tmp_filename[256];
}config_t;

extern config_t g_config;

typedef enum enum_event_type_tag
{
    CALL_SEND = 101,
    CALL_RECV = 102,
    SMS_SEND  = 201,
    SMS_RECV  = 202,
    UPDATE_LEAVE   = 301,
    UPDATE_ENTER   = 302,
    UPDATE_REGULAR = 303,
    CELL_OUT  = 401,
    CELL_IN   = 402,
    MOBILE_OPEN  = 501,
    MOBILE_CLOSE = 502,
    ROAM_OUT = 601,
    MMS_SEND = 901,
    MMS_RECV = 902,
}enum_event_type;

int read_config(const char * cfg_file);
int read_decode_map(const char * file);
char * get_line(FILE * f, char * line, int len);

#endif
