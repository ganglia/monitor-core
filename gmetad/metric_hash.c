/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -G -l -H metric_hash -t -F ', 0' -N in_metric_list -k '1,6,$' -W metrics ./metric_list  */
/* $Id$ */
#include <gmetad.h>

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
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64,  5, 64, 30, 20,  0,
      23,  0,  0, 64, 10, 18, 64, 64,  0,  0,
      29, 15,  0, 64, 10, 10, 27,  0, 64,  0,
      64,  5, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64
    };
  return len + asso_values[(unsigned char)str[5]] + asso_values[(unsigned char)str[0]] + asso_values[(unsigned char)str[len - 1]];
}

static unsigned char lengthtable[] =
  {
     0,  0,  0,  0,  0,  0,  6,  0,  0,  9, 10,  0,  0,  0,
     9, 10,  6,  0,  8,  9,  0, 11,  0,  8,  0,  0,  8,  0,
     8,  0, 10,  8,  9, 10,  9,  0, 13, 10, 11,  0,  7, 12,
     0, 10,  0,  0,  0,  8,  0,  0,  8,  0,  0,  0,  7,  0,
     0,  0,  0,  0, 10,  9,  8, 10
  };

struct ganglia_metric metrics[] =
  {
    {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
    {"wcache", FLOAT},
    {"", 0}, {"", 0},
    {"load_five", FLOAT},
    {"lwrite_sec", FLOAT},
    {"", 0}, {"", 0}, {"", 0},
    {"lread_sec", FLOAT},
    {"cpu_system", FLOAT},
    {"rcache", FLOAT},
    {"", 0},
    {"mem_free", UINT32},
    {"swap_free", UINT32},
    {"", 0},
    {"mem_buffers", UINT32},
    {"", 0},
    {"load_one", FLOAT},
    {"", 0}, {"", 0},
    {"cpu_nice", FLOAT},
    {"", 0},
    {"cpu_user", FLOAT},
    {"", 0},
    {"bwrite_sec", FLOAT},
    {"cpu_idle", FLOAT},
    {"disk_free", DOUBLE},
    {"phread_sec", FLOAT},
    {"bread_sec", FLOAT},
    {"", 0},
    {"part_max_used", FLOAT},
    {"proc_total", UINT32},
    {"phwrite_sec", FLOAT},
    {"", 0},
    {"cpu_wio", FLOAT},
    {"load_fifteen", FLOAT},
    {"", 0},
    {"mem_shared", UINT32},
    {"", 0}, {"", 0}, {"", 0},
    {"proc_run", UINT32},
    {"", 0}, {"", 0},
    {"pkts_out", FLOAT},
    {"", 0}, {"", 0}, {"", 0},
    {"pkts_in", FLOAT},
    {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
    {"disk_total", DOUBLE},
    {"bytes_out", FLOAT},
    {"bytes_in", FLOAT},
    {"mem_cached", UINT32}
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
