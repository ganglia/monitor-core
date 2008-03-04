/* C code produced by gperf version 3.0.2 */
/* Command-line: gperf -l -H xml_hash -t -F ', 0' -N in_xml_list -k '1,$' -W xml_tags ./xml_hash.gperf  */
/* $Id$ */
#include <gmetad.h>

#define TOTAL_KEYWORDS 32
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
      45, 45, 45, 45, 45,  5, 45,  0, 15,  0,
      45,  0, 30, 10, 45, 45,  0, 20, 10,  0,
      25, 45, 15,  0,  0,  5,  0, 45, 20, 30,
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
       0,  0,  0,  3,  4,  5,  6,  7,  3,  9,  5, 11,  2, 13,
       4, 10,  0,  7,  8,  4,  5,  0,  7,  3,  4,  0,  6,  7,
      13,  4,  0,  0,  2,  3,  4,  5,  0,  2,  8,  4,  0,  0,
       0,  0,  9
    };
  static struct xml_tag xml_tags[] =
    {
      {"", 0}, {"", 0}, {"", 0},
      {"VAL", VAL_TAG},
      {"TYPE", TYPE_TAG},
      {"SLOPE", SLOPE_TAG},
      {"SOURCE", SOURCE_TAG},
      {"LATLONG", LATLONG_TAG,},
      {"URL", URL_TAG,},
      {"LOCALTIME", LOCALTIME_TAG},
      {"UNITS", UNITS_TAG},
      {"GANGLIA_XML", GANGLIA_XML_TAG},
      {"TN", TN_TAG},
      {"EXTRA_ELEMENT", EXTRA_ELEMENT_TAG},
      {"NAME", NAME_TAG},
      {"EXTRA_DATA", EXTRA_DATA_TAG},
      {"", 0},
      {"VERSION", VERSION_TAG},
      {"LOCATION", LOCATION_TAG},
      {"GRID", GRID_TAG},
      {"OWNER", OWNER_TAG,},
      {"", 0},
      {"CLUSTER", CLUSTER_TAG},
      {"SUM", SUM_TAG,},
      {"TMAX", TMAX_TAG},
      {"", 0},
      {"METRIC", METRIC_TAG},
      {"METRICS", METRICS_TAG},
      {"GMOND_STARTED", STARTED_TAG},
      {"DOWN", DOWN_TAG,},
      {"", 0}, {"", 0},
      {"UP", UP_TAG,},
      {"NUM", NUM_TAG,},
      {"HOST", HOST_TAG},
      {"HOSTS", HOSTS_TAG,},
      {"", 0},
      {"IP", IP_TAG},
      {"REPORTED", REPORTED_TAG},
      {"DMAX", DMAX_TAG},
      {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"AUTHORITY", AUTHORITY_TAG}
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
