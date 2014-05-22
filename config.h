#ifndef __CONFIG_H__
#define __CONFIG_H__

#define def_decode(x) int field_##x = -1
#define export_decode(x) extern int field_##x
#define decode(x) field_##x

#define LEN_LACCELL (12)
#define LEN_LINE (1024)
#define CONTEXT_BASESIZE      (1000000ul)
#define CONTEXT_SORT_BASESIZE (100000ul)
#define CONTEXT_PART (10)

#define APP_NAME "saCellContext"

#define MAX(a, b) ((a) > (b)? (a): (b))
#define MIN(a, b) ((a) < (b)? (a): (b))

export_decode(imsi);
export_decode(timestamp);
export_decode(event_type);
export_decode(lac);
export_decode(cell);
export_decode(msisdn);
export_decode(imei);

extern int num_of_field;

struct config_t {
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
};
typedef struct config_t config_t;

extern config_t g_config;
#define CFG(x) (g_config._##x)

enum enum_event_type {
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
};
typedef enum enum_event_type enum_event_type;

int read_config(const char * cfg_file);
int read_decode_map(const char * file);

#endif /* __CONFIG_H__ */

