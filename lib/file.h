#ifndef FILE_H
#define FILE_H 1

#include <sys/time.h>

/* Never changes */
#ifndef BUFFSIZE
#define BUFFSIZE 4096
#endif

typedef struct {
  struct timeval last_read;
  float thresh;
  char *name;
  char *buffer;
  size_t buffersize;
} timely_file;

/* FreeBSD seems to gag on these.. Yet still works when not compiled in */
#if defined(BSD)
ssize_t readn (int fd, void *vptr, size_t n);
ssize_t writen (int fd, const void *vptr, size_t n);
#endif
int slurpfile (char *filename, char **buffer, int buflen);
char *skip_whitespace (const char *p);
char *skip_token (const char *p);

#endif
