/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
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
