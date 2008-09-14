#ifndef FILE_H
#define FILE_H 1

/* FreeBSD seems to gag on these.. Yet still works when not compiled in */
#if defined(BSD)
ssize_t readn (int fd, void *vptr, size_t n);
ssize_t writen (int fd, const void *vptr, size_t n);
#endif
int slurpfile ( char * filename, char **buffer, int buflen );
char *skip_whitespace (const char *p);
char *skip_token (const char *p);

#endif
