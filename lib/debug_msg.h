#ifndef DEBUG_MSG_H
#define DEBUG_MSG_H 1

void debug_msg(const char *format, ...); 
void set_debug_msg_level(int level);
int get_debug_msg_level();

#endif
