/**
 * @file file.c Some file functions
 */
/* $Id$ */
#include "ganglia_private.h"

/**
 * @fn ssize_t readn (int fd, void *vptr, size_t n)
 * Reads "n" bytes from a descriptor
 * @param fd The descriptor
 * @param vptr A void pointer to a data buffer
 * @param n The data buffer size
 * @return ssize_t
 * @retval 0 on success
 * @retval -1 on failure
 */
ssize_t                         
readn (int fd, void *vptr, size_t n)
{
   size_t nleft;
   ssize_t nread;
   char *ptr;
 
   ptr = vptr;
   nleft = n;
   while (nleft > 0)
     {
        nread = read (fd, ptr, nleft);
        if (nread < 0)
          {
             if (errno == EINTR)
                nread = 0;      /* and call read() again */
             else
                return SYNAPSE_FAILURE;
          }
        else if (nread == 0)
           break;               /* EOF */
 
        nleft -= nread;
        ptr   += nread;
     }
   return (n - nleft);          /* return >= 0 */
}                                                 

/**
 * @fn ssize_t writen (int fd, const void *vptr, size_t n)
 * Writes "n" bytes from a descriptor
 * @param fd The descriptor
 * @param vptr A void pointer to a data buffer
 * @param n The data buffer size
 * @return ssize_t
 * @retval 0 on success
 * @retval -1 on failure
 */
ssize_t 
writen (int fd, const void *vptr, size_t n)
{
   size_t nleft;
   ssize_t nwritten;
   const char *ptr;
 
   ptr = vptr;
   nleft = n;
   while (nleft > 0)
     {
        nwritten = write (fd, ptr, nleft);
        if (nwritten <= 0)
          {
             if (errno == EINTR)
                nwritten = 0;   /* and call write() again */
             else
                return SYNAPSE_FAILURE;
          }
        nleft -= nwritten;
        ptr += nwritten;
     }
   return SYNAPSE_SUCCESS;
}                     

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
         return SYNAPSE_FAILURE;
      }
 
  read:
   read_len = read( fd, buffer, buflen );
   if ( read_len <= 0 )
      {
         if ( errno == EINTR )
            goto read;
         err_ret("slurpfile() read() error");
         close(fd);
         return SYNAPSE_FAILURE;
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

