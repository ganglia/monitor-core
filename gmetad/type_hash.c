/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -G -l -H type_hash -t -F ', 0' -N in_type_list -k '1,$' -W types ./type_hash.gperf  */
/* $Id$ */
/* Recognizes metric types. 
 * Build with: gperf -G -l -H type_hash -t -F ', 0' -N in_type_list -k 1,$ \
 * -W types ./type_hash.gperf > type_hash.c
 */
#include <gmetad.h>

#define TOTAL_KEYWORDS 10
#define MIN_WORD_LENGTH 4
#define MAX_WORD_LENGTH 9
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 21
/* maximum key range = 18, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
type_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      10, 22, 22, 22,  5, 22,  0, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      15,  0, 15,  0, 22,  0, 22, 22, 22, 22,
      22, 22,  0, 22, 22,  0,  0,  0, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

static unsigned char lengthtable[] =
  {
     0,  0,  0,  0,  4,  5,  6,  0,  0,  9,  5,  6,  0,  0,
     0,  5,  6,  0,  0,  0,  5,  6
  };

static struct type_tag types[] =
  {
    {"", 0}, {"", 0}, {"", 0}, {"", 0},
    {"int8", INT},
    {"uint8", UINT},
    {"string", STRING},
    {"", 0}, {"", 0},
    {"timestamp", TIMESTAMP},
    {"int16", INT},
    {"uint16", UINT},
    {"", 0}, {"", 0}, {"", 0},
    {"int32", INT},
    {"uint32", UINT},
    {"", 0}, {"", 0}, {"", 0},
    {"float", FLOAT},
    {"double", FLOAT}
  };

#ifdef __GNUC__
__inline
#endif
struct type_tag *
in_type_list (str, len)
     register const char *str;
     register unsigned int len;
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = type_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        if (len == lengthtable[key])
          {
            register const char *s = types[key].name;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &types[key];
          }
    }
  return 0;
}
