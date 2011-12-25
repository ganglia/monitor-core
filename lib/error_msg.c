/**
 * @file error_msg.c Error Handling Functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#ifndef MAXLINE
#define MAXLINE 4096
#endif

int daemon_proc;    /* set nonzero by daemon_init() */

int ganglia_quiet_errors = 0;

static void err_doit (int, int, const char *, va_list);

void
err_quiet( void )
{
   ganglia_quiet_errors = 1;
}

void
err_verbose( void )
{
   ganglia_quiet_errors = 0;
}

/**
 * @fn void err_ret (const char *fmt, ...)
 * Print a message and return. Nonfatal error related to a system call.
 * @param fmt Format string the same as printf function
 * @param ... Arguments for the format string
 */
void
err_ret (const char *fmt, ...)
{
   va_list ap;

   va_start (ap, fmt);
   err_doit (1, LOG_INFO, fmt, ap);
   va_end (ap);
   return;
}

/**
 * @fn void err_sys (const char *fmt, ...)
 * Print a message and terminate.
 * Fatal error related to a system call.
 * @param fmt Format string the same as printf function
 * @param ... Arguments for the format string
 */
void
err_sys (const char *fmt, ...)
{
   va_list ap;

   va_start (ap, fmt);
   err_doit (1, LOG_ERR, fmt, ap);
   va_end (ap);
   exit (1);
}

/**
 * @fn void err_dump (const char *fmt, ...)
 * Print a message, dump core, and terminate.
 * Fatal error related to a system call.
 * @param fmt Format string the same as printf function
 * @param ... Arguments for the format string
 */
void
err_dump (const char *fmt, ...)
{
   va_list ap;

   va_start (ap, fmt);
   err_doit (1, LOG_ERR, fmt, ap);
   va_end (ap);
   abort ();			/* dump core and terminate */
   exit (1);			/* shouldn't get here */
}

/**
 * @fn void err_msg (const char *fmt, ...)
 * Print a message and return. Nonfatal error unrelated to a system call.
 * @param fmt Format string the same as printf function
 * @param ... Arguments for the format string
 */
void
err_msg (const char *fmt, ...)
{
   va_list ap;

   va_start (ap, fmt);
   err_doit (0, LOG_INFO, fmt, ap);
   va_end (ap);
   return;
}

/**
 * @fn void err_quit (const char *fmt, ...)
 * Print a message and terminate. Fatal error unrelated to a system call.
 * @param fmt Format string the same as printf function
 * @param ... Arguments for the format string
 */
void
err_quit (const char *fmt, ...)
{
   va_list ap;

   va_start (ap, fmt);
   err_doit (0, LOG_ERR, fmt, ap);
   va_end (ap);
   exit (1);
}

/* Print a message and return to caller.
 * Caller specifies "errnoflag" and "level". */

static void
err_doit (int errnoflag, int level, const char *fmt, va_list ap)
{
   int errno_save, n;
   char buf[MAXLINE + 1];

   if(ganglia_quiet_errors)
      return;

   errno_save = errno;		/* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
   vsnprintf (buf, MAXLINE, fmt, ap);	/* safe */
#else
   vsprintf (buf, fmt, ap);	/* not safe */
#endif
   n = strlen (buf);
   if (errnoflag)
      snprintf (buf + n, MAXLINE - n, ": %s", strerror (errno_save));
#ifdef HAVE_STRLCAT
   strlcat (buf, "\n", MAXLINE);
#else
   strcat (buf, "\n");
#endif

   if (daemon_proc)
     {
        syslog (level, "%s", buf);
     }
   else
     {
        fflush (stdout);	/* in case stdout and stderr are the same */
        fputs (buf, stderr);
        fflush (stderr);
     }
   return;
}
