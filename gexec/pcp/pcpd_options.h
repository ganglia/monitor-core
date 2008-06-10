/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __PCPD_OPTIONS_H
#define __PCPD_OPTIONS_H

typedef struct {
    int inetd;
    int port;
    int daemon;
} pcpd_options;

void pcpd_options_init(pcpd_options *opts);
void pcpd_options_parse(pcpd_options *opts, int argc, char **argv);

#endif /* __PCPD_OPTIONS_H */
