/**
 * @file debug_msg.c Debug Message function
 */
/* $Id$ */
#include "gangliaconf.h"

int debug_level = 0;

/**
 * @fn void debug_msg(const char *format, ...)
 * Prints the message to STDERR if DEBUG is #defined
 * @param format The format of the msg (see printf manpage)
 * @param ... Optional arguments
 */
void
debug_msg(const char *format, ...)
{
   if (debug_level)
      {
         va_list ap;
         va_start(ap, format);
         vfprintf(stderr, format, ap);
         fprintf(stderr,"\n");
         va_end(ap);
      } 
   return;
}
