#ifndef BECOME_A_NOBODY_H
#define BECOME_A_NOBODY_H 1
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

void become_a_nobody(const char *username);

#endif
