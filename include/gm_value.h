#ifndef GM_VALUE_H
#define GM_VALUE_H 1

#include <inttypes.h>

enum g_type_t {
   g_string,  /* huh uh.. he said g string */
   g_int8,
   g_uint8,
   g_int16,
   g_uint16,
   g_int32,
   g_uint32,
   g_float,
   g_double,
   g_timestamp    /* a uint32 */
};
typedef enum g_type_t g_type_t;
 
#define MAX_G_STRING_SIZE 64
 
typedef union {
    int8_t   int8;
   uint8_t  uint8;
   int16_t  int16;
  uint16_t uint16;
   int32_t  int32;
  uint32_t uint32;
   float   f; 
   double  d;
   char str[MAX_G_STRING_SIZE];
} g_val_t;         

#endif  /* GM_VALUE_H */
