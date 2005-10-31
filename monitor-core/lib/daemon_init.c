/**
 * @file daemon_init.c Functions for standalone daemons
 */ 
/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "daemon_init.h"

#define	MAXFD	64

/**
 * @fn void update_pidfile (const char *pname)
 * @param argv0 name of this program
 */
extern pid_t getpgid(pid_t pid);
void
update_pidfile (char *pidfile)
{
  pid_t pid;
  mode_t prev_umask;
  FILE *file;

  /* make sure this program isn't already running. */
  file = fopen (pidfile, "r");
  if (file)
    {
      if (fscanf(file, "%d", &pid) == 1 &&
	  (getpgid (pid) > -1))
	{
	  fprintf (stderr, "daemon already running: %s pid %d\n", pidfile, pid);
	  exit (1);
	}
      fclose (file);
    }

  /* write the pid of this process to the pidfile */
  prev_umask = umask (0112);
  unlink(pidfile);

  file = fopen (pidfile, "w");
  if (!file)
    {
      fprintf (stderr, "Error writing pidfile '%s' -- %s\n",
	       pidfile, strerror (errno));
      exit (1);
    }
  fprintf (file, "%d\n", (int) getpid());
  fclose (file);
  umask (prev_umask);
}


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
