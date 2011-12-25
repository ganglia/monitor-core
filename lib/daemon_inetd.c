/**
 * @file daemon_inetd.c Functions for inetd daemons
 */
#include <stdarg.h>
#include <syslog.h>

extern int daemon_proc;		/* defined in error_msg.c */

/**
 * @fn void daemon_inetd (const char *pname, int facility)
 * Initialize a inetd daemon and syslog functions
 * @param pname The name of your program
 * @param facility Specify the type of program logging (man openlog for details)
 */
void
daemon_inetd (const char *pname, int facility)
{
   daemon_proc = 1;		/* for our err_XXX() functions */
   openlog (pname, LOG_PID, facility);
}
