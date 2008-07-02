#ifndef GM_MSG_H
#define GM_MSG_H 1

#ifdef __cplusplus
extern "C" {
#endif

extern int ganglia_quiet_errors;

void debug_msg(const char *format, ...); 
void set_debug_msg_level(int level);
int get_debug_msg_level();

void err_quiet( void );
void err_ret (const char *fmt, ...);
void err_sys (const char *fmt, ...);
void err_dump (const char *fmt, ...);
void err_msg (const char *fmt, ...);
void err_quit (const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
