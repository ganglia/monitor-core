/**
 * @file daemon_init.c Functions for standalone daemons
 */ 
/* $Id$ */
#include	"gangliaconf.h"
#include "daemon_init.h"

#define	MAXFD	64

extern int daemon_proc;		/* defined in error.c */

/**
 * @fn void daemon_init (const char *pname, int facility)
 * @param pname The name of your program
 * @param facility See the openlog() manpage for details
 */
void
daemon_init (const char *pname, int facility)
{
   int i;
   pid_t pid;

   pid = fork();

   if (pid != 0)
      exit (0);			/* parent terminates */

   /* 41st child continues */
   setsid ();			/* become session leader */

   signal (SIGHUP, SIG_IGN);
   if ((pid = fork ()) != 0)
      exit (0);			/* 1st child terminates */

   /* 42nd child continues */
   daemon_proc = 1;		/* for our err_XXX() functions */

   chdir ("/");			/* change working directory */

   umask (0);			/* clear our file mode creation mask */

   for (i = 0; i < MAXFD; i++)
      close (i);

   openlog (pname, LOG_PID, facility);
}
