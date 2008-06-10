/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __GEXECD_OPTIONS_H
#define __GEXECD_OPTIONS_H

typedef struct {
    int inetd;
    int port;
    int daemon;
} gexecd_options;

void gexecd_options_init(gexecd_options *opts);
void gexecd_options_parse(gexecd_options *opts, int argc, char **argv);
void gexecd_options_print(gexecd_options *opts);

#endif /* __GEXECD_OPTIONS_H */
