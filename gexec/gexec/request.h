/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __REQUEST_H
#define __REQUEST_H

#include <unistd.h>
#include <sys/types.h>
#include "gexec_lib.h"

typedef struct {
    int              argc;
    char             **argv;
    int              envc;
    char             **envp;
    _gexec_gpid      gpid;
    char             *user;
    char             *cwd;
    gexec_spawn_opts opts;
} request;

void request_create(request **r, int argc, char **argv, int envc, char **envp,
                    gexec_spawn_opts *opts);
int  request_create_from_buf(request **r, void *buf, int buf_len);
void *request_malloc_buf(request *r, int *buf_len);
void request_destroy(request *r);

void request_print(request *r);

#endif /* __REQUEST_H */
