/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <e/bytes.h>
#include <e/cpa.h>
#include <e/e_error.h>
#include <e/net.h>
#include <e/safe.h>
#include <e/xmalloc.h>
#include "gexec_lib.h"
#include "request.h"

/* GPIDs are 32 bit host IP address : seconds : microseconds */
static void init_gpid(_gexec_gpid *gpid)
{
    struct timeval tv;
    struct hostent *h;
    char           ip[16]; 

    gettimeofday(&tv, NULL);

    net_gethostip(ip);
    h = gethostbyname(ip);
    e_assert(h != NULL);

    memcpy(&gpid->addr.s_addr, h->h_addr_list[0], sizeof(struct in_addr));
    gpid->sec  = tv.tv_sec;
    gpid->usec = tv.tv_usec;
}

void request_create(request **r, int argc, char **argv, int envc, char **envp,
                    gexec_spawn_opts *opts)
{
    char          *dir;
    struct passwd *pw;

    *r = (request *)xmalloc(sizeof(request));
    cpa_create(&(*r)->argv, &(*r)->argc, argv, argc);
    cpa_create(&(*r)->envp, &(*r)->envc, envp, envc);
    init_gpid(&(*r)->gpid);
    pw = getpwuid(getuid());
    (*r)->user = (char *)xmalloc(strlen(pw->pw_name) + NULLBYTE_SIZE);
    strcpy((*r)->user, pw->pw_name);
    dir = get_current_dir_name();
    e_assert(dir != NULL);
    (*r)->cwd = (char *)xmalloc(strlen(dir) + NULLBYTE_SIZE);
    strcpy((*r)->cwd, dir);
    xfree(dir);
    memcpy(&(*r)->opts, opts, sizeof(gexec_spawn_opts));
}

int request_create_from_buf(request **r, void *buf, int buf_len)
{
    void *p;
    int  argv_buf_len, envp_buf_len, user_len, cwd_len, plen;

    p    = buf;
    plen = buf_len;
    
    *r = (request *)xmalloc(sizeof(request));
    (*r)->argv = NULL;
    (*r)->envp = NULL;

    if (safe_memcpy(&argv_buf_len, p, sizeof(argv_buf_len), plen) != E_OK)
        goto cleanup;
    p    += sizeof(argv_buf_len);
    plen -= sizeof(argv_buf_len);
    if (cpa_create_from_buf(&(*r)->argv, &(*r)->argc, p, argv_buf_len) != E_OK)
        goto cleanup;
    p    += argv_buf_len;
    plen -= argv_buf_len;

    if (safe_memcpy(&envp_buf_len, p, sizeof(envp_buf_len), plen) != E_OK)
        goto cleanup;
    p    += sizeof(envp_buf_len);
    plen -= sizeof(envp_buf_len);
    if (cpa_create_from_buf(&(*r)->envp, &(*r)->envc, p, envp_buf_len) != E_OK)
        goto cleanup;
    p    += envp_buf_len;
    plen -= envp_buf_len;

    if (safe_memcpy(&(*r)->gpid, p, sizeof(_gexec_gpid), plen) != E_OK)
        goto cleanup;
    p    += sizeof(_gexec_gpid);
    plen -= sizeof(_gexec_gpid);

    if (safe_memcpy(&user_len, p, sizeof(user_len), plen) != E_OK)
        goto cleanup;
    p    += sizeof(user_len);
    plen -= sizeof(user_len);
    (*r)->user = (char *)xmalloc(user_len);
    if (safe_memcpy((*r)->user, p, user_len, plen) != E_OK)
        goto cleanup;
    p    += user_len;
    plen -= user_len;

    if (safe_memcpy(&cwd_len, p, sizeof(cwd_len), plen) != E_OK)
        goto cleanup;
    p    += sizeof(cwd_len);
    plen -= sizeof(cwd_len);
    (*r)->cwd = (char *)xmalloc(cwd_len);
    if (safe_memcpy((*r)->cwd, p, cwd_len, plen) != E_OK)
        goto cleanup;
    p    += cwd_len;
    plen -= cwd_len;

    if (safe_memcpy(&(*r)->opts, p, sizeof(gexec_spawn_opts), plen) != E_OK)
        goto cleanup;
    p    += sizeof(gexec_spawn_opts);
    plen -= sizeof(gexec_spawn_opts);

    e_assert(plen == 0);
    return E_OK;

 cleanup:
    if ((*r)->envp != NULL)
        cpa_destroy((*r)->envp, (*r)->envc);
    if ((*r)->argv != NULL)
        cpa_destroy((*r)->argv, (*r)->argc);
    xfree(*r);
    return E_MEMLEN_ERROR;
}

void *request_malloc_buf(request *r, int *buf_len)
{
    void *buf, *p, *argv_buf, *envp_buf;
    int  argv_buf_len, envp_buf_len;
    int  user_len, cwd_len;
    
    argv_buf = cpa_malloc_buf(r->argv, r->argc, &argv_buf_len);
    envp_buf = cpa_malloc_buf(r->envp, r->envc, &envp_buf_len);
    user_len = strlen(r->user) + NULLBYTE_SIZE;
    cwd_len  = strlen(r->cwd) + NULLBYTE_SIZE;

    *buf_len = 
        sizeof(argv_buf_len) + argv_buf_len +
        sizeof(envp_buf_len) + envp_buf_len +
        sizeof(_gexec_gpid) +
        sizeof(user_len) + user_len +
        sizeof(cwd_len) + cwd_len +
        sizeof(gexec_spawn_opts);
    buf = xmalloc(*buf_len);
    p = buf;

    memcpy(p, &argv_buf_len, sizeof(argv_buf_len));
    p += sizeof(argv_buf_len);

    memcpy(p, argv_buf, argv_buf_len);
    p += argv_buf_len;

    memcpy(p, &envp_buf_len, sizeof(envp_buf_len));
    p += sizeof(envp_buf_len);

    memcpy(p, envp_buf, envp_buf_len);
    p += envp_buf_len;

    memcpy(p, &r->gpid, sizeof(_gexec_gpid));
    p += sizeof(_gexec_gpid);

    memcpy(p, &user_len, sizeof(user_len));
    p += sizeof(user_len);

    memcpy(p, r->user, user_len);
    p += user_len;

    memcpy(p, &cwd_len, sizeof(cwd_len));
    p += sizeof(cwd_len);

    memcpy(p, r->cwd, cwd_len);
    p += cwd_len;

    memcpy(p, &r->opts, sizeof(gexec_spawn_opts));
    p += sizeof(gexec_spawn_opts);

    xfree(envp_buf);
    xfree(argv_buf);
    return buf;
}

void request_destroy(request *r)
{
    cpa_destroy(r->argv, r->argc);
    cpa_destroy(r->envp, r->envc);
    xfree(r->user);
    xfree(r->cwd);
    xfree(r);
}

void request_print(request *r)
{
    int i;

    for (i = 0; i < r->argc + 1; i++)
        printf("argv[%d] = %s\n", i, r->argv[i]);
    for (i = 0; i < r->envc + 1; i++)
        printf("envp[%d] = %s\n", i, r->envp[i]);
    printf("gpid                = 0x%8x:%ld:%ld\n", r->gpid.addr.s_addr,
           r->gpid.sec, r->gpid.usec);
    printf("user                = %s\n", r->user);
    printf("cwd                 = %s\n", r->cwd);
    printf("opts.verbose        = %d\n", r->opts.verbose); 
    printf("opts.detached       = %d\n", r->opts.detached);
    printf("opts.port           = %d\n", r->opts.port);
    printf("opts.tree_fanout    = %d\n", r->opts.tree_fanout);
    printf("opts.prefix_type    = %d\n", r->opts.prefix_type);
}
