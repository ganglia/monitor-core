/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -l -H xml_hash -t -F ', 0' -N in_xml_list -k '1,$' -W xml_tags ./xml_hash.gperf  */
/* $Id$ */
/* Including gmetad.h for the xml_tag_t definition */
#include <gmetad.h>
struct xml_tag
   {
      const char *name;
      xml_tag_t tag;
   };

#define TOTAL_KEYWORDS 10
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 11
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 19
/* maximum key range = 17, duplicates = 0 */

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
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20,  0, 20,  0,
      20,  0,  5, 20, 20, 20,  0,  0, 15, 20,
      20, 20,  0,  0,  0, 20,  0, 20, 10, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20
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
       0,  0,  0,  3,  4,  5,  6,  7,  0,  4,  0, 11,  0,  0,
       4,  0,  0,  2,  0,  4
    };
  static struct xml_tag xml_tags[] =
    {
      {"", 0}, {"", 0}, {"", 0},
      {"VAL", VAL_TAG},
      {"TYPE", TYPE_TAG},
      {"SLOPE", SLOPE_TAG},
      {"METRIC", METRIC_TAG},
      {"CLUSTER", CLUSTER_TAG},
      {"", 0},
      {"HOST", HOST_TAG},
      {"", 0},
      {"GANGLIA_XML", GANGLIA_XML_TAG},
      {"", 0}, {"", 0},
      {"TMAX", TMAX_TAG},
      {"", 0}, {"", 0},
      {"TN", TN_TAG},
      {"", 0},
      {"NAME", NAME_TAG}
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
