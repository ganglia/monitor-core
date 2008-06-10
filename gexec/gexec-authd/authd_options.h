/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __AUTHD_OPTIONS_H
#define __AUTHD_OPTIONS_H

typedef struct {
    int daemon;
} authd_options;

void authd_options_init(authd_options *opts);
void authd_options_parse(authd_options *opts, int argc, char **argv);

#endif /* __AUTHD_OPTIONS_H */
