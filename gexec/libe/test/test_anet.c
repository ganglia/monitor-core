#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "anet.h"
#include "e_error.h"
#include "xmalloc.h"

int main(int argc, char **argv)
{
    int tcpsock;

    if (anet_cli_tcpsock_create(&tcpsock) != E_OK) {
        fatal_error("Failed to create TCP socket\n");
        exit(1);
    };
    anet_cli_tcpsock_destroy(tcpsock);
    return 0;
}
