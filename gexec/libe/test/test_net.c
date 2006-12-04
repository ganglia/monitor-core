#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "e_error.h"
#include "net.h"
#include "xmalloc.h"

int main(int argc, char **argv)
{
    char ip[16];
    
    net_hosttoip("www.cs.berkeley.edu", ip);
    printf("%s\n", ip);
    return 0;
}
