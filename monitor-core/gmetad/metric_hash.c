/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -G -l -H metric_hash -t -F ', 0' -N in_metric_list -k '1,6,$' -W metrics ./metric_hash.gperf  */
/* $Id$ */
/* Included for the metric_tyep definition */
#include <gmetad.h>

#define TOTAL_KEYWORDS 33
#define MIN_WORD_LENGTH 6
#define MAX_WORD_LENGTH 13
#define MIN_HASH_VALUE 6
#define MAX_HASH_VALUE 72
/* maximum key range = 67, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
unsigned int
metric_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 10, 73, 20, 25,  0,
      35,  0,  5, 73, 10, 50, 73, 73,  0,  0,
      10, 15,  0, 73, 10, 15, 15,  5, 73,  0,
      73,  5, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73
    };
  return len + asso_values[(unsigned char)str[5]] + asso_values[(unsigned char)str[0]] + asso_values[(unsigned char)str[len - 1]];
}

static unsigned char lengthtable[] =
  {
     0,  0,  0,  0,  0,  0,  6,  0,  0,  0, 10,  0,  7,  0,
     9, 10,  6,  0,  8,  9,  0,  0,  0,  8,  9, 10, 11, 12,
     8,  9,  0, 11,  0,  8,  0, 10,  0,  0,  8,  0, 10,  0,
     0,  8,  9, 10,  0,  0, 13,  9,  0,  0,  0,  8,  0, 10,
     0,  0,  8,  9, 10,  0,  0,  0,  0, 10,  0,  7,  0,  0,
     0,  0,  7
  };

struct ganglia_metric metrics[] =
  {
    {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
    {"wcache", FLOAT},
    {"", 0}, {"", 0}, {"", 0},
    {"lwrite_sec", FLOAT},
    {"", 0},
    {"cpu_num", UINT32},
    {"", 0},
    {"load_five", FLOAT},
    {"cpu_system", FLOAT},
    {"rcache", FLOAT},
    {"", 0},
    {"mem_free", UINT32},
    {"lread_sec", FLOAT},
    {"", 0}, {"", 0}, {"", 0},
    {"load_one", FLOAT},
    {"mem_total", UINT32},
    {"proc_total", UINT32},
    {"phwrite_sec", FLOAT},
    {"load_fifteen", FLOAT},
    {"proc_run", UINT32},
    {"swap_free", UINT32},
    {"", 0},
    {"mem_buffers", UINT32},
    {"", 0},
    {"cpu_user", FLOAT},
    {"", 0},
    {"bwrite_sec", FLOAT},
    {"", 0}, {"", 0},
    {"pkts_out", FLOAT},
    {"", 0},
    {"swap_total", UINT32},
    {"", 0}, {"", 0},
    {"cpu_idle", FLOAT},
    {"bread_sec", FLOAT},
    {"phread_sec", FLOAT},
    {"", 0}, {"", 0},
    {"part_max_used", FLOAT},
    {"disk_free", DOUBLE},
    {"", 0}, {"", 0}, {"", 0},
    {"bytes_in", FLOAT},
    {"", 0},
    {"mem_shared", UINT32},
    {"", 0}, {"", 0},
    {"cpu_nice", FLOAT},
    {"bytes_out", FLOAT},
    {"disk_total", DOUBLE},
    {"", 0}, {"", 0}, {"", 0}, {"", 0},
    {"mem_cached", UINT32},
    {"", 0},
    {"pkts_in", FLOAT},
    {"", 0}, {"", 0}, {"", 0}, {"", 0},
    {"cpu_wio", FLOAT}
  };

#ifdef __GNUC__
__inline
#endif
struct ganglia_metric *
in_metric_list (str, len)
     register const char *str;
     register unsigned int len;
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = metric_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        if (len == lengthtable[key])
          {
            register const char *s = metrics[key].name;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &metrics[key];
          }
    }
  return 0;
}
