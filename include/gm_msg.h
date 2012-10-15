#ifndef GM_MSG_H
#define GM_MSG_H 1

#ifdef __GNUC__
#  define CHECK_FMT(posf,posv) __attribute__((format(printf, posf, posv)))
#else
#  define CHECK_FMT(posf,posv)
#endif // __GNUC__

#ifdef __cplusplus
extern "C" {
#endif

extern int ganglia_quiet_errors;

void debug_msg(const char *format, ...) CHECK_FMT(1,2);
void set_debug_msg_level(int level);
int get_debug_msg_level();

void err_quiet( void );
void err_ret (const char *fmt, ...) CHECK_FMT(1,2);
void err_sys (const char *fmt, ...) CHECK_FMT(1,2);
void err_dump (const char *fmt, ...) CHECK_FMT(1,2);
void err_msg (const char *fmt, ...) CHECK_FMT(1,2);
void err_quit (const char *fmt, ...) CHECK_FMT(1,2);

#ifdef __cplusplus
}
#endif

#endif
