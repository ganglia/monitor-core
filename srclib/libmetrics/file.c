/**
 * @file file.c Some file functions
 */
/* $Id$ */
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>


#include "file.h"
#include "error.h"

/**
 * @fn int slurpfile ( char * filename, char *buffer, int buflen )
 * Reads an entire file into a buffer
 * @param filename The name of the file to read into memory
 * @param buffer A pointer to the data buffer
 * @param buflen The data buffer length
 * @return int
 * @retval 0 on success
 * @retval -1 on failure
 */
int
slurpfile ( char * filename, char *buffer, int buflen )
{
   int  fd, read_len;
 
   fd = open(filename, O_RDONLY);
   if ( fd < 0 )
      {
         err_ret("slurpfile() open() error");      
         return -1;
      }
 
  read:
   read_len = read( fd, buffer, buflen );
   if ( read_len <= 0 )
      {
         if ( errno == EINTR )
            goto read;
         err_ret("slurpfile() read() error");
         close(fd);
         return -1;
      }
   close(fd);

   buffer[read_len] = '\0';
   return read_len;
}   


char * 
skip_whitespace ( const char *p)
{
    while (isspace(*p)) p++;
    return (char *)p;
}
 
char * 
skip_token ( const char *p)
{
    while (isspace(*p)) p++;
    while (*p && !isspace(*p)) p++;
    return (char *)p;
}         

