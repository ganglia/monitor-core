/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -G -l -k 1,4,7 -H gmetad_hash -N in_metric_list -W metrics ./metric_list  */
#include "gmetad.h"

#define TOTAL_KEYWORDS 43
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 13
#define MIN_HASH_VALUE 6
/* maximum key range = 92, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
gmetad_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98,  0, 98, 35,  0,  0,
       0, 15,  0, 10,  5, 35, 98, 60, 10, 30,
      50, 10,  0, 98,  0, 40, 35, 30, 98,  5,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
      98, 98, 98, 98, 98, 98
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      case 6:
      case 5:
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static unsigned char lengthtable[] =
  {
     0,  0,  0,  0,  0,  0,  6,  0,  8,  0,  0,  6,  0,  0,
     0,  0,  0,  7,  8,  0, 10,  0,  0,  8,  9, 10, 11,  0,
     0,  9,  5,  0,  0,  3,  9, 10,  0,  7,  8,  0, 10, 11,
     0,  0,  0, 10,  0,  0,  0,  9, 10,  0,  0,  8,  9, 10,
     0, 12,  8,  9, 10,  0, 12,  8,  0,  0,  0,  0,  8,  9,
     0,  0,  0,  8,  9, 10,  0,  0,  8,  0, 10,  0,  7, 13,
     9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  9,  0,  0,  7
  };

static const char * metrics[] =
  {
    "", "", "", "", "", "",
    "rcache",
    "",
    "cpu_nice",
    "", "",
    "wcache",
    "", "", "", "", "",
    "cpu_wio",
    "cpu_idle",
    "",
    "proc_total",
    "", "",
    "cpu_user",
    "cpu_speed",
    "phread_sec",
    "phwrite_sec",
    "", "",
    "heartbeat",
    "gexec",
    "", "",
    "mtu",
    "bytes_out",
    "os_release",
    "",
    "cpu_num",
    "proc_run",
    "",
    "mem_cached",
    "mem_buffers",
    "", "", "",
    "bwrite_sec",
    "", "", "",
    "swap_free",
    "cpu_system",
    "", "",
    "mem_free",
    "load_five",
    "lwrite_sec",
    "",
    "load_fifteen",
    "bytes_in",
    "sys_clock",
    "swap_total",
    "",
    "machine_type",
    "location",
    "", "", "", "",
    "load_one",
    "disk_free",
    "", "", "",
    "boottime",
    "mem_total",
    "mem_shared",
    "", "",
    "pkts_out",
    "",
    "disk_total",
    "",
    "os_name",
    "part_max_used",
    "bread_sec",
    "", "", "", "", "", "", "", "", "",
    "lread_sec",
    "", "",
    "pkts_in"
  };

#ifdef __GNUC__
__inline
#endif
const char *
in_metric_list (str, len)
     register const char *str;
     register unsigned int len;
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = gmetad_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        if (len == lengthtable[key])
          {
            register const char *s = metrics[key];

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return s;
          }
    }
  return 0;
}
