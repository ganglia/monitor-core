/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <e/e_error.h>
#include <e/net.h>
#include <e/xmalloc.h>
#include "pcp.h"
#include "pcp_lib.h"
#include "pcp_options.h"

static void create_pcp_write_args(pcp_write_args **w_args, int argc, 
                                  char **argv, int args_index)
{
    char    *host;
    int     i, j;
    int     nargs = argc - args_index;

    if (nargs < 3) {
        printf("Usage: pcp localfile remotefile host0 [host1] [host2] ..\n");
        exit(3);
    }

    i = args_index;
    (*w_args) = (pcp_write_args *)xmalloc(sizeof(pcp_write_args));
    (*w_args)->localfile  = argv[i++];
    (*w_args)->remotefile = argv[i++];
    (*w_args)->nhosts     = argc - i; 
    (*w_args)->ips        = (char **)xmalloc((*w_args)->nhosts * sizeof(char *));

    for (j = 0; j < (*w_args)->nhosts; j++) {
        (*w_args)->ips[j] = (char *)xmalloc(IP_STRLEN);
        host = argv[i++];
        if (net_hosttoip(host, (*w_args)->ips[j]) != E_OK) {
            fprintf(stderr, "Bad host %s\n", host);
            exit(2);
        }
    }
}

static void create_pcp_delete_args(pcp_delete_args **d_args, int argc, 
                                   char **argv, int args_index)
{
    char    *host;
    int     i, j;
    int     nargs = argc - args_index;

    if (nargs < 2) {
        printf("Usage: pcp remotefile host0 [host1] [host2] ..\n");
        exit(3);
    }

    i = args_index;
    (*d_args) = (pcp_delete_args *)xmalloc(sizeof(pcp_delete_args));
    (*d_args)->remotefile = argv[i++];
    (*d_args)->nhosts     = argc - i; 
    (*d_args)->ips        = (char **)xmalloc((*d_args)->nhosts * sizeof(char *));

    for (j = 0; j < (*d_args)->nhosts; j++) {
        (*d_args)->ips[j] = (char *)xmalloc(IP_STRLEN);
        host = argv[i++];
        if (net_hosttoip(host, (*d_args)->ips[j]) != E_OK) {
            fprintf(stderr, "Bad host %s\n", host);
            exit(2);
        }
    }
}

static void create_pcp_checksum_args(pcp_checksum_args **c_args, int argc, 
                                     char **argv, int args_index)
{
    char    *host;
    int     i, j;
    int     nargs = argc - args_index;

    if (nargs < 2) {
        printf("Usage: pcp remotefile host0 [host1] [host2] ..\n");
        exit(3);
    }

    i = args_index;
    (*c_args) = (pcp_checksum_args *)xmalloc(sizeof(pcp_checksum_args));
    (*c_args)->remotefile = argv[i++];
    (*c_args)->nhosts     = argc - i; 
    (*c_args)->ips        = (char **)xmalloc((*c_args)->nhosts * sizeof(char *));

    for (j = 0; j < (*c_args)->nhosts; j++) {
        (*c_args)->ips[j] = (char *)xmalloc(IP_STRLEN);
        host = argv[i++];
        if (net_hosttoip(host, (*c_args)->ips[j]) != E_OK) {
            fprintf(stderr, "Bad host %s\n", host);
            exit(2);
        }
    }
}

static void set_cmd_opts(pcp_options *opts, pcp_cmd_options *cmd_opts)
{
    cmd_opts->verbose       = opts->verbose;
    cmd_opts->port          = opts->port;
    cmd_opts->fanout        = opts->fanout;
    cmd_opts->frag_size     = opts->frag_size;
    cmd_opts->best_effort   = opts->best_effort;
}

int main(int argc, char **argv)
{
    pcp_options         opts;
    pcp_cmd_options     cmd_opts;
    pcp_write_args      *w_args;
    pcp_delete_args     *d_args;
    pcp_checksum_args   *c_args;

    pcp_options_init(&opts);
    pcp_options_read_pcprc(&opts);
    pcp_options_parse(&opts, argc, argv);
    set_cmd_opts(&opts, &cmd_opts);

    switch (opts.cmd_type) {

    case PCP_WRITE_CMD: /* default */
        create_pcp_write_args(&w_args, argc, argv, opts.args_index);
        pcp_write(w_args->nhosts, w_args->ips, w_args->localfile, w_args->remotefile, 
                  &cmd_opts);
        break;

    case PCP_DELETE_CMD:
        create_pcp_delete_args(&d_args, argc, argv, opts.args_index);
        pcp_delete(d_args->nhosts, d_args->ips, d_args->remotefile, &cmd_opts);
        break;

    case PCP_CHECKSUM_CMD:
        create_pcp_checksum_args(&c_args, argc, argv, opts.args_index);
        pcp_checksum(c_args->nhosts, c_args->ips, c_args->remotefile, &cmd_opts);
        break;
    }

    return 0;
}
