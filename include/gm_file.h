#ifndef FILE_H
#define FILE_H 1

#include <sys/time.h>

/* Never changes */
#ifndef BUFFSIZE
#define BUFFSIZE 65536
#endif

typedef struct {
  struct timeval last_read;
  float thresh;
  char *name;
  char *buffer;
  size_t buffersize;
} timely_file;

#define SLURP_FAILURE -1

#ifdef __cplusplus
extern "C" {
#endif

int slurpfile (char *filename, char **buffer, int buflen);
float timediff (const struct timeval *thistime,
                const struct timeval *lasttime);
char *update_file(timely_file *tf);
char *skip_whitespace (const char *p);
char *skip_token (const char *p);

#ifdef __cplusplus
}
#endif

#endif
