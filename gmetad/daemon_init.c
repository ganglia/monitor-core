/**
 * @file daemon_init.c Functions for standalone daemons
 */ 
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "daemon_init.h"

#define MAXFD 64

extern int daemon_proc;		/* defined in error_msg.c */

/**
 * @fn void daemon_init (const char *pname, int facility)
 * @param pname The name of your program
 * @param facility See the openlog() manpage for details
 */
void
daemon_init (const char *pname, int facility, mode_t rrd_umask)
{
   int i;
   pid_t pid;

   pid = fork();

   if (pid != 0)
      exit (0);         /* parent terminates */

   /* 41st child continues */
   setsid ();           /* become session leader */

   signal (SIGHUP, SIG_IGN);
   if ((pid = fork ()) != 0)
      exit (0);         /* 1st child terminates */

   /* 42nd child continues */
   daemon_proc = 1;     /* for our err_XXX() functions */

   i = chdir ("/");     /* change working directory */

   /* filter out rrd file priviledges using umask */
   umask (rrd_umask);

   for (i = 0; i < MAXFD; i++)
      close (i);

   openlog (pname, LOG_PID, facility);
}
