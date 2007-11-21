/* C code produced by gperf version 3.0.1 */
/* Command-line: gperf -l -H xml_hash -t -F ', 0' -N in_xml_list -k '1,$' -W xml_tags ./xml_hash.gperf  */
/* $Id$ */
#include <gmetad.h>

#define TOTAL_KEYWORDS 34
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 13
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 55
/* maximum key range = 53, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
xml_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56,  0, 56,  5, 15,  0,
      56,  0, 35, 15, 56, 56,  0, 20, 10,  5,
      30, 56, 20, 15,  0,  5,  0, 56, 30, 35,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
struct xml_tag *
in_xml_list (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char lengthtable[] =
    {
       0,  0,  0,  3,  4,  5,  0,  7,  3,  9, 10, 11,  2,  0,
       4,  0,  0,  7,  8,  4,  5,  6,  0,  0,  4,  5,  0,  0,
      13,  4,  5,  6,  7,  3,  4,  5,  0,  2,  3,  4,  0,  0,
       7,  8,  9,  0,  0,  2,  0,  4,  0,  0,  0,  0,  0,  5
    };
  static struct xml_tag xml_tags[] =
    {
      {"", 0}, {"", 0}, {"", 0},
      {"VAL", VAL_TAG},
      {"TYPE", TYPE_TAG},
      {"TITLE", TITLE_TAG,},
      {"", 0},
      {"LATLONG", LATLONG_TAG,},
      {"URL", URL_TAG,},
      {"LOCALTIME", LOCALTIME_TAG},
      {"EXTRA_DATA", EXTRA_DATA_TAG},
      {"GANGLIA_XML", GANGLIA_XML_TAG},
      {"TN", TN_TAG},
      {"", 0},
      {"NAME", NAME_TAG},
      {"", 0}, {"", 0},
      {"VERSION", VERSION_TAG},
      {"LOCATION", LOCATION_TAG},
      {"GRID", GRID_TAG},
      {"SLOPE", SLOPE_TAG},
      {"SOURCE", SOURCE_TAG},
      {"", 0}, {"", 0},
      {"DESC", DESC_TAG,},
      {"UNITS", UNITS_TAG},
      {"", 0}, {"", 0},
      {"GMOND_STARTED", STARTED_TAG},
      {"DOWN", DOWN_TAG,},
      {"OWNER", OWNER_TAG,},
      {"METRIC", METRIC_TAG},
      {"CLUSTER", CLUSTER_TAG},
      {"NUM", NUM_TAG,},
      {"TMAX", TMAX_TAG},
      {"GROUP", GROUP_TAG},
      {"", 0},
      {"UP", UP_TAG,},
      {"SUM", SUM_TAG,},
      {"HOST", HOST_TAG},
      {"", 0}, {"", 0},
      {"METRICS", METRICS_TAG},
      {"REPORTED", REPORTED_TAG},
      {"AUTHORITY", AUTHORITY_TAG},
      {"", 0}, {"", 0},
      {"IP", IP_TAG},
      {"", 0},
      {"DMAX", DMAX_TAG},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"HOSTS", HOSTS_TAG,}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = xml_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        if (len == lengthtable[key])
          {
            register const char *s = xml_tags[key].name;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &xml_tags[key];
          }
    }
  return 0;
}
