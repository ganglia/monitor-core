/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -l -H xml_hash -t -F ', 0' -N in_xml_list -k '1,$' -W xml_tags ./xml_hash.gperf  */
/* $Id$ */
#include <gmetad.h>


#define TOTAL_KEYWORDS 30
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 13
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 44
/* maximum key range = 42, duplicates = 0 */

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
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 20, 45,  5, 15,  0,
      45, 15, 20, 15, 45, 45,  0,  0, 12,  5,
       0, 45,  0,  0,  0, 30, 25, 45, 23,  0,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45
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
       0,  0,  0,  3,  4,  5,  6,  7,  0,  9,  5,  6,  7,  0,
       2,  3,  4,  2,  0,  0,  8,  0,  7,  8,  4,  5, 11,  4,
       3,  9,  0,  4,  2,  3,  4,  5,  0,  0,  0,  0,  0,  0,
       4, 13,  7
    };
  static struct xml_tag xml_tags[] =
    {
      {"", 0}, {"", 0}, {"", 0},
      {"SUM", SUM_TAG,},
      {"TYPE", TYPE_TAG},
      {"SLOPE", SLOPE_TAG},
      {"SOURCE", SOURCE_TAG},
      {"METRICS", METRICS_TAG},
      {"", 0},
      {"LOCALTIME", LOCALTIME_TAG},
      {"OWNER", OWNER_TAG,},
      {"METRIC", METRIC_TAG},
      {"CLUSTER", CLUSTER_TAG},
      {"", 0},
      {"TN", TN_TAG},
      {"NUM", NUM_TAG,},
      {"NAME", NAME_TAG},
      {"IP", IP_TAG},
      {"", 0}, {"", 0},
      {"LOCATION", LOCATION_TAG},
      {"", 0},
      {"LATLONG", LATLONG_TAG,},
      {"REPORTED", REPORTED_TAG},
      {"HOST", HOST_TAG},
      {"HOSTS", HOSTS_TAG,},
      {"GANGLIA_XML", GANGLIA_XML_TAG},
      {"TMAX", TMAX_TAG},
      {"VAL", VAL_TAG},
      {"AUTHORITY", AUTHORITY_TAG},
      {"", 0},
      {"DOWN", DOWN_TAG,},
      {"UP", UP_TAG,},
      {"URL", URL_TAG,},
      {"GRID", GRID_TAG},
      {"UNITS", UNITS_TAG},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"DMAX", DMAX_TAG},
      {"GMOND_STARTED", STARTED_TAG},
      {"VERSION", VERSION_TAG}
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
