/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __GEXEC_OPTIONS_H
#define __GEXEC_OPTIONS_H

#include "gexec_lib.h"

/* The gexec client may not support setting of all spawn options */
typedef struct {
    int              nprocs;
    gexec_spawn_opts spawn_opts;
} gexec_options;

void gexec_options_init(gexec_options *opts);
void gexec_options_parse(gexec_options *opts, int argc, char **argv, 
                         int *args_index);
void gexec_options_print(gexec_options *opts);

#endif /* __GEXEC_OPTIONS_H */
