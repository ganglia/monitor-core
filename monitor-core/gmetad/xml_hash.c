/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -l -H xml_hash -t -F ', 0' -N in_xml_list -k '1,$' -W xml_tags ./xml_hash.gperf  */
/* $Id$ */
#include <gmetad.h>


#define TOTAL_KEYWORDS 26
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 13
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 47
/* maximum key range = 44, duplicates = 0 */

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
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48,  5, 48, 15, 15,  0,
      48,  0, 20, 25, 48, 48,  0,  5, 30,  5,
       0, 48,  0,  0,  0, 15, 10, 48, 25,  0,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48
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
       0,  0,  0,  0,  4,  5,  6,  7,  0,  9,  5, 11,  7,  3,
       9,  0,  0,  0,  3,  4,  5,  0,  7,  8,  4,  5,  6,  2,
      13,  4,  0,  0,  2,  0,  4,  0,  0,  0,  8,  0,  0,  0,
       0,  0,  4,  0,  0,  7
    };
  static struct xml_tag xml_tags[] =
    {
      {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"TYPE", TYPE_TAG},
      {"SLOPE", SLOPE_TAG},
      {"SOURCE", SOURCE_TAG},
      {"LATLONG", LATLONG_TAG,},
      {"", 0},
      {"LOCALTIME", LOCALTIME_TAG},
      {"OWNER", OWNER_TAG,},
      {"GANGLIA_XML", GANGLIA_XML_TAG},
      {"METRICS", METRICS_TAG},
      {"VAL", VAL_TAG},
      {"AUTHORITY", AUTHORITY_TAG},
      {"", 0}, {"", 0}, {"", 0},
      {"URL", URL_TAG,},
      {"GRID", GRID_TAG},
      {"UNITS", UNITS_TAG},
      {"", 0},
      {"CLUSTER", CLUSTER_TAG},
      {"REPORTED", REPORTED_TAG},
      {"HOST", HOST_TAG},
      {"HOSTS", HOSTS_TAG},
      {"METRIC", METRIC_TAG},
      {"IP", IP_TAG},
      {"GMOND_STARTED", STARTED_TAG},
      {"TMAX", TMAX_TAG},
      {"", 0}, {"", 0},
      {"TN", TN_TAG},
      {"", 0},
      {"NAME", NAME_TAG},
      {"", 0}, {"", 0}, {"", 0},
      {"LOCATION", LOCATION_TAG},
      {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0},
      {"DMAX", DMAX_TAG},
      {"", 0}, {"", 0},
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
