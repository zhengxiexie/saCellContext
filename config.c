#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "log.h"

config_t  g_config;
int		  num_of_field = 0;

const int cfgmap_output_interval[] = { 3600, 1800, 3600, 7200, 86400};


DEF_DECODE(imsi);
DEF_DECODE(timestamp);
DEF_DECODE(event_type);
DEF_DECODE(lac);
DEF_DECODE(cell);
DEF_DECODE(msisdn);
DEF_DECODE(imei);

int read_config(const char * filename)
{
    FILE * cfg_file;
    char   word[2][CONFIG_KEY_VALUE_LENGTH];
    char   line[CONFIG_LENGTH];
    int    flag_indq = 1;
    int    idx_w = 0;
    int    pos_w = 0;
    int    pos   = 0;

    cfg_file = fopen(filename, "r");
    if (!cfg_file) return -1;

    while (get_line(cfg_file, line, CONFIG_LENGTH) != NULL)
   	{
        word[0][0] = '\0';
        word[1][0] = '\0';

        // get word[0] and word[1]
        if (line[0] == '\0') continue;
        pos   = 0;
        idx_w = 0;
        pos_w = 0;
        flag_indq = 0;
        memset(word[0], 0, CONFIG_KEY_VALUE_LENGTH);
        memset(word[1], 0, CONFIG_KEY_VALUE_LENGTH);
        while ((idx_w < 2) && (line[pos]))
	   	{
            if (line[pos] == '\"') {
                flag_indq = 1 - flag_indq;
                pos++;
            }
            if (line[pos] == '\t' || line[pos] == ' ')
		   	{
                if (flag_indq) {
                    word[idx_w][pos_w++] = line[pos];
                    pos++;
                } else {
                    word[idx_w++][pos_w + 1] = 0;
                    pos_w = 0;
					while (line[pos] == '\t' || line[pos] == ' '){
						if (line[pos]) pos++;
					}
                }
            } else {
                word[idx_w][pos_w++] = line[pos];
                pos++;
            }
        }

        GET_STR_CFG(output_dir);
        GET_STR_CFG(output_prefix);
        GET_STR_CFG(output_suffix);
        GET_STR_CFG(read_dir);
        GET_STR_CFG(tmp_filename);
        GET_STR_CFG(backup_dir);
        GET_INT_CFG(sort_min);
        GET_INT_CFG(sort_buffer);
        GET_INT_CFG(sleep_interval);
        GET_MAP_INT_CFG(output_interval);
        GET_INT_CFG(context_size);
        GET_INT_CFG(context_thread);
        GET_INT_CFG(cleanup_mark);
        GET_INT_CFG(tz_offset);
        GET_INT_CFG(cleanup_min);
        GET_INT_CFG(cleanup_hour);
        GET_INT_CFG(cross_mountpoint);

    }

    fclose(cfg_file);

    // post processing config
    g_config._context_size *= CONTEXT_BASESIZE;
    g_config._sort_buffer  *= CONTEXT_SORT_BASESIZE;
    /* g_config._output_interval = (g_config._output_interval > SIZE_OF(output_interval_val) || g_config._output_interval < 0)?  3600: output_interval_val[g_config._output_interval]; logmsg(stdout, "output interval = %d", g_config._output_interval); */
    return 0;
}

int read_decode_map( const char * filename )
{
    FILE * f;
    char line[DECODE_LINE_LENGTH];
    char * comma;

    f = fopen(filename, "r");
    if (!f) return 1;

    while (get_line(f, line, DECODE_LINE_LENGTH) != NULL)
   	{
		if (line[0] == 0){
			continue;
		}

        comma = strchr(line, ',');
		if (!comma){
			continue;
		}

        *comma = 0;
        comma ++;

		GET_DECODE(imsi);
		GET_DECODE(timestamp);
		GET_DECODE(event_type);
		GET_DECODE(lac);
		GET_DECODE(cell);
		GET_DECODE(msisdn);
		GET_DECODE(imei);
    }

    logmsg(stdout, "num_of_field = %d", num_of_field);
    fclose(f);
    return 0;
}

char * get_line(FILE * f, char * line, int len)
{
    char * ret = fgets(line, len, f);
    int i = 0;
    if (!ret) return NULL;

    // comment?
    if (ret[0] == '#') ret[0] = 0;

    // trim any EOL
    for (i = strlen(ret) - 1; i >= 0; i--)
   	{
        if (ret[i] == '\r' || ret[i] == '\n')
			ret[i] = 0;
    }
    return ret;
}
