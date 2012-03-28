/**
 * @file file.c Some file functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "gm_file.h"

#include "ganglia_priv.h"

/**
 * @fn int slurpfile ( char * filename, char **buffer, int buflen )
 * Reads an entire file into a buffer
 * @param filename The name of the file to read into memory
 * @param buffer A pointer reference to the data buffer
 * @param buflen The data buffer length
 * @return int
 * @retval number of bytes read on success
 * @retval -1 on failure
 */
int
slurpfile (char * filename, char **buffer, int buflen)
{
   int fd, read_len;
   int dynamic = 0, sl = 0;
   char *db;

   fd = open(filename, O_RDONLY);
   if (fd < 0)
      {
         err_ret("slurpfile() open() error on file %s", filename);
         return SLURP_FAILURE;
      }
   if (*buffer == NULL)
      {
         db = malloc(buflen);
         dynamic = buflen;
         *buffer = db;
      }
   else
      db = *buffer;

read:
   read_len = read(fd, db, buflen);
   if (read_len < 0)
      {
         if (errno == EINTR)
            goto read;
         err_ret("slurpfile() read() error on file %s", filename);
         close(fd);
         return SLURP_FAILURE;
      }
   else
      sl += read_len;

   if (read_len == buflen)
      {
         if (dynamic) {
            dynamic += buflen;
            db = realloc(*buffer, dynamic);
            *buffer = db;
            db = *buffer + dynamic - buflen;
            goto read;
         } else {
            --read_len;
            err_msg("slurpfile() read() buffer overflow on file %s", filename);
         }
      }
   db[read_len] = '\0';

   close(fd);
   return sl;
}

float timediff (const struct timeval *thistime,
                const struct timeval *lasttime)
{
  float diff;

  diff = ((double) thistime->tv_sec * 1.0e6 +
          (double) thistime->tv_usec -
          (double) lasttime->tv_sec * 1.0e6 -
          (double) lasttime->tv_usec) / 1.0e6;

  return diff;
}

char *
update_file (timely_file *tf)
{
    int rval;
    struct timeval now;
    char *bp;

    gettimeofday(&now, NULL);
    if(timediff(&now,&tf->last_read) > tf->thresh) {
        bp = tf->buffer;
        rval = slurpfile(tf->name, &bp, tf->buffersize);
        if(rval == SLURP_FAILURE) {
            err_msg("update_file() got an error from slurpfile() reading %s",
                    tf->name);
        } else {
            tf->last_read = now;
            if (tf->buffer == NULL) {
               tf->buffer = bp;
               if (rval > tf->buffersize)
                  tf->buffersize = ((rval/tf->buffersize) + 1) * tf->buffersize;
            }
        }
    }
    return tf->buffer;
}

char *
skip_whitespace (const char *p)
{
    while (isspace((unsigned char)*p)) p++;
    return (char *)p;
}

char *
skip_token (const char *p)
{
    while (isspace((unsigned char)*p)) p++;
    while (*p && !isspace((unsigned char)*p)) p++;
    return (char *)p;
}
