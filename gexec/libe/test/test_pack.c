#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "pack.h"
#include "xmalloc.h"

int main(int argc, char **argv)
{
    u_char  c1, c2;
    u_short s1, s2;
    u_long  l1, l2;
    u_char  *buf;
    u_int   len;

    len = 2 * (sizeof(u_char) + sizeof(u_short) + sizeof(u_long));
    buf = xmalloc(len);
    bzero(buf, len);

    c1 = 'a';
    c2 = 'b';
    s1 = 1;
    s2 = 2;
    l1 = 33;
    l2 = 44;

    pack(buf, "csllsc", c1, s1, l1, l2, s2, c2);

    c1 = '\0';
    c2 = '\0';
    s1 = 0;
    s2 = 0;
    l1 = 0;
    l2 = 0;

    unpack(buf, "csllsc", &c1, &s1, &l1, &l2, &s2, &c2);

    printf("c1 = %c\n", c1);
    printf("c2 = %c\n", c2);
    printf("s1 = %ho\n", s1);
    printf("s2 = %ho\n", s2);
    printf("l1 = %lo\n", l1);
    printf("l2 = %lo\n", l2);
    
    xfree(buf);
    return 0;
}
