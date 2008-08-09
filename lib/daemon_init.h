#ifndef DAEMON_INIT_H
#define DAEMON_INIT_H 1

void daemon_init (const char *pname, int facility); 
void update_pidfile (char *pidfile);

#endif
