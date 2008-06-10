/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <e/bitmask.h>
#include <e/bytes.h>
#include <e/e_error.h>
#include <e/net.h>
#include <e/token_array.h>
#include <e/xmalloc.h>
#include <e/llist.h>
#include "gexec.h"
#include "gexec_lib.h"
#include "gexec_options.h"
#ifdef GANGLIA
#include <ganglia.h>
extern void err_quiet();
#endif /* GANGLIA */

extern char **environ;

static void print_bad_nodes(int *bad_nodes, char **ips)
{
    char    host[1024];
    char    *ip; 
    int     bad_node;
    int     bad_nodes_copy[GEXEC_MAX_NPROCS];

    bitmask_copy(bad_nodes_copy, bad_nodes, GEXEC_MAX_NPROCS);
    while ((bad_node = bitmask_ffs(bad_nodes_copy, GEXEC_MAX_NPROCS)) != -1) {
        ip = ips[bad_node];
        net_iptohost(ip, host, 1024);
        printf("Could not connect to %s (%s)\n", host, ip);
        bitmask_clr(bad_nodes_copy, GEXEC_MAX_NPROCS, bad_node);
    }
}

#ifdef GANGLIA

static void set_gexec_svrs(char **ips, int nhosts)
{
    char *gexec_svrs, *p;
    int  i, len;

    p = gexec_svrs = xmalloc(nhosts * IP_STRLEN);
    for (i = 0; i < nhosts; i++) {
        len = strlen(ips[i]);
        e_assert(len < IP_STRLEN);
        strcpy(p, ips[i]);
        p += len;
        *p++ = ' ';
    }
    *(--p) = '\0';
    if (setenv("GEXEC_SVRS", gexec_svrs, 1) < 0) {
        fatal_error("setenv error");
        exit(1);
    }
}

static void svr_addrs_create(char ***svr_ips, int **svr_ports, int *nsvrs, 
                             char *gmond_svrs)
{
    int     i, n_svr_tokens;
    char    **svrs, **svr_tokens;
    char    *host;

    token_array_create(&svrs, nsvrs, gmond_svrs, ' ');
    *svr_ips   = (char **)xmalloc(*nsvrs * sizeof(char *));
    *svr_ports = (int *)xmalloc(*nsvrs * sizeof(int));
    for (i = 0; i < *nsvrs; i++) {
        token_array_create(&svr_tokens, &n_svr_tokens, svrs[i], ':');
        if (n_svr_tokens == 1) {
            host = svr_tokens[0];
            (*svr_ports)[i] = GEXEC_GANGLIA_PORT;
        }
        else {
            host = svr_tokens[0];
            (*svr_ports)[i] = atoi(svr_tokens[1]);
        }
        (*svr_ips)[i] = (char *)xmalloc(IP_STRLEN);
        if (net_hosttoip(host, (*svr_ips)[i]) != E_OK) {
            fprintf(stderr, "Bad gmond server %s\n", svrs[i]);
            exit(2);
        }
        token_array_destroy(svr_tokens, n_svr_tokens);
    }
    token_array_destroy(svrs, *nsvrs);
}

static void svr_addrs_destroy(char **svr_ips, int *svr_ports, int nsvrs)
{
    int i;

    xfree(svr_ports);
    for (i = 0; i < nsvrs; i++)
        xfree(svr_ips[i]);
    xfree(svr_ips);
}

#endif /* GANGLIA */

static void ips_create_explicit(char ***ips, int *nhosts, char *gexec_svrs)
{
    int     i;
    char    **hosts;

    token_array_create(&hosts, nhosts, gexec_svrs, ' ');
    *ips = (char **)xmalloc(*nhosts * sizeof(char *));
    for (i = 0; i < *nhosts; i++) {
        (*ips)[i] = (char *)xmalloc(IP_STRLEN);
        if (net_hosttoip(hosts[i], (*ips)[i]) != E_OK) {
            fprintf(stderr, "Bad host %s\n", hosts[i]);
            exit(2);
        }
    }
    token_array_destroy(hosts, *nhosts);
}

static void ips_create_ganglia(char ***ips, int *nhosts, char *gmond_svrs)
{
#ifndef GANGLIA
    printf("Node selection using Ganglia not supported\n");
    exit(3);
#else
    int             i, j, nsvrs, svr_alive;
    int             *svr_ports;
    char            **svr_ips;
    gexec_cluster_t cluster;
    gexec_host_t    *host;
    llist_entry     *lli;

    svr_alive = 0;
    *nhosts   = 0;

    err_quiet();
    svr_addrs_create(&svr_ips, &svr_ports, &nsvrs, gmond_svrs);
    for (i = 0; i < nsvrs; i++) {
        if (gexec_cluster(&cluster, svr_ips[i], svr_ports[i]) != 0)
            continue;
        svr_alive = 1;
        if (cluster.num_gexec_hosts == 0) {
            gexec_cluster_free(&cluster);
            continue;
        }
        *nhosts = cluster.num_gexec_hosts;
        *ips    = (char **)xmalloc(*nhosts * sizeof(char *));
        lli = cluster.gexec_hosts;
        for (j = 0; j < *nhosts; j++) {
            e_assert(lli != NULL);
            (*ips)[j] = (char *)xmalloc(IP_STRLEN);
            host = (gexec_host_t *)lli->val;
            e_assert(strlen(host->ip) < IP_STRLEN);
            strcpy((*ips)[j], host->ip);
            lli = lli->next;
        }
        gexec_cluster_free(&cluster);
        break;
    }
    svr_addrs_destroy(svr_ips, svr_ports, nsvrs);

    if (!svr_alive) {
        printf("All GEXEC_GMOND_SVRS servers are down\n");
        exit(3);
    }
    if (svr_alive && *nhosts == 0) {
        printf("All GEXEC_GMOND_SVRS servers have no hosts available\n");
        exit(3);
    }
#endif /* GANGLIA */
}

#ifdef GANGLIA
static int ips_create_local_ganglia(char ***ips, int *nhosts)
{
    int             i;
    gexec_cluster_t cluster;
    gexec_host_t    *host;
    llist_entry     *lli;

    *nhosts = 0;
    err_quiet();
    if (gexec_cluster(&cluster, "127.0.0.1", GEXEC_GANGLIA_PORT) != 0)
        return GEXEC_NO_HOSTS_ERROR;
    if (cluster.num_gexec_hosts == 0) {
        gexec_cluster_free(&cluster);
        return GEXEC_NO_HOSTS_ERROR;
    }
    *nhosts = cluster.num_gexec_hosts;
    *ips    = (char **)xmalloc(*nhosts * sizeof(char *));
    lli = cluster.gexec_hosts;
    for (i = 0; i < *nhosts; i++) {
        e_assert(lli != NULL);
        (*ips)[i] = (char *)xmalloc(IP_STRLEN);
        host = (gexec_host_t *)lli->val;
        e_assert(strlen(host->ip) < IP_STRLEN);
        strcpy((*ips)[i], host->ip);
        lli = lli->next;
    }
    gexec_cluster_free(&cluster);
    return GEXEC_OK;
}
#endif /* GANGLIA */

static void ips_create(char ***ips, int *nhosts)
{
    char *gexec_svrs, *gmond_svrs;

    if ((gexec_svrs = getenv("GEXEC_SVRS")) != NULL)
        ips_create_explicit(ips, nhosts, gexec_svrs);
    else if ((gmond_svrs = getenv("GEXEC_GMOND_SVRS")) != NULL)
        ips_create_ganglia(ips, nhosts, gmond_svrs);
    else {
#ifdef GANGLIA
        if (ips_create_local_ganglia(ips, nhosts) == GEXEC_OK)
            return;
#endif /* GANGLIA */
        fprintf(stderr, "Must set either GEXEC_SVRS or GEXEC_GMOND_SVRS\n");
        exit(3);
    }
}

static void ips_destroy(char **ips, int nhosts)
{
    int i;
    
    for (i = 0; i < nhosts; i++)
        xfree(ips[i]);
    xfree(ips);
}

static char *malloc_fullpath(char *path)
{
    char        *fullpath = NULL;
    char        *path_env;
    char        **dirs;
    int         ndirs, i, maxdir, found_path = 0;
    struct stat stat_buf;
 
    if (path[0] == '/') {
        if (stat(path, &stat_buf) < 0) {
            fprintf(stderr, "Bad filename %s\n", path);
            exit(3);
        }
        fullpath = (char *)xmalloc(strlen(path) + NULLBYTE_SIZE);
        strcpy(fullpath, path);
        return fullpath;
    }
    if ((path_env = getenv("PATH")) == NULL) {
        fprintf(stderr, "PATH not set\n");
        exit(3);
    }
    token_array_create(&dirs, &ndirs, path_env, ':');
    maxdir   = strlen(token_array_maxtoken(dirs, ndirs));
    fullpath = xmalloc(maxdir + SLASHBYTE_SIZE + strlen(path) + NULLBYTE_SIZE);
    for (i = 0; i < ndirs; i++) {
        sprintf(fullpath, "%s/%s", dirs[i], path);
        if (stat(fullpath, &stat_buf) == 0 &&
            (stat_buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
            found_path = 1;
            break;
        }
    }
    token_array_destroy(dirs, ndirs);
    if (!found_path) {
        fprintf(stderr, "Bad filename %s\n", path);
        exit(3);
    }
    return fullpath;
}

static void gargs_create(int argc, char **argv, int *gargc, char ***gargv, int args_index)
{
    int i, len;

    *gargc = argc - args_index;
    *gargv = (char **)xmalloc((*gargc + 1) * sizeof(char *));
    (*gargv)[0] = malloc_fullpath(argv[args_index]);
    for (i = 1; i < *gargc; i++) {
        len = strlen(argv[args_index + i]);
        (*gargv)[i] = xmalloc(len + NULLBYTE_SIZE);
        strcpy((*gargv)[i], argv[args_index + i]);
    }
    (*gargv)[i] = NULL;
}

static void gargs_destroy(int gargc, char **gargv)
{
    int i;

    for (i = 0; i < gargc; i++)
        xfree(gargv[i]);
    xfree(gargv);
}

int main(int argc, char **argv)
{
    gexec_options opts;
    int           bad_nodes[GEXEC_BITMASK_SIZE];
    int           gargc, args_index, nhosts;
    char          **ips, **gargv;
    
    gexec_init();

    gexec_options_init(&opts);
    gexec_options_parse(&opts, argc, argv, &args_index);

    ips_create(&ips, &nhosts);

    gargs_create(argc, argv, &gargc, &gargv, args_index);
    if (opts.nprocs == 0)
        opts.nprocs = nhosts;
    if (opts.nprocs > nhosts) {
        fprintf(stderr, "Not enough hosts available\n");
        exit(3);
    }
#ifdef GANGLIA
    set_gexec_svrs(ips, opts.nprocs);
#endif /* GANGLIA */
    gexec_spawn(opts.nprocs, ips, gargc, gargv, environ, 
                &opts.spawn_opts, bad_nodes);
    print_bad_nodes(bad_nodes, ips);
    gargs_destroy(gargc, gargv);

    ips_destroy(ips, nhosts);

    gexec_shutdown();

    return 0;
}
