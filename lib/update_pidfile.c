/**
 * @file update_pidfile.c Functions for standalone daemons
 */
#define _XOPEN_SOURCE 500 /* for getpgid */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gm_msg.h>
#include "update_pidfile.h"

/**
 * @fn void update_pidfile (const char *pname)
 * @param argv0 name of this program
 */
void
update_pidfile (char *pidfile)
{
  long p;
  pid_t pid;
  mode_t prev_umask;
  FILE *file;

  /* make sure this program isn't already running. */
  file = fopen (pidfile, "r");
  if (file)
    {
      if (fscanf(file, "%ld", &p) == 1 && (pid = p) &&
          (getpgid (pid) > -1))
        {
	  if (pid != getpid())
	    {
              err_msg("daemon already running: %s pid %ld\n", pidfile, p);
              exit (1);
	    }
	  else
	    {
	      /* We have the same PID so were probably HUP'd */
	      return;
	    }
        }
      fclose (file);
    }

  /* write the pid of this process to the pidfile */
  prev_umask = umask (0112);
  unlink(pidfile);

  file = fopen (pidfile, "w");
  if (!file)
    {
      err_msg("Error writing pidfile '%s' -- %s\n",
               pidfile, strerror (errno));
      exit (1);
    }
  fprintf (file, "%ld\n", (long) getpid());
  fclose (file);
  umask (prev_umask);
}
