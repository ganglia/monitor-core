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
