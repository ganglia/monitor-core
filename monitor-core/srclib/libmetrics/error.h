#ifndef ERROR_H
#define ERROR_H 1

void err_quiet( void );
void err_verbose( void );
void err_ret (const char *fmt, ...);
void err_sys (const char *fmt, ...);
void err_dump (const char *fmt, ...);
void err_msg (const char *fmt, ...);
void err_quit (const char *fmt, ...);

#endif

