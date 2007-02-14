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
#ifndef __PCP_OPTIONS_H
#define __PCP_OPTIONS_H

typedef struct {
    int cmd_type;
    int verbose;
    int port;
    int fanout;
    int frag_size;
    int best_effort;
    int args_index;
} pcp_options;

void pcp_options_init(pcp_options *opts);
void pcp_options_read_pcprc(pcp_options *opts);
void pcp_options_parse(pcp_options *opts, int argc, char **argv);

#endif /* __PCP_OPTIONS_H */
