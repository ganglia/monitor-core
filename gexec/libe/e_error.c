/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include "e_error.h"

void _e_assert(int expression, char *file, int line)
{
    if (!expression) {
        fprintf(stderr, "%s:%d Assertion failed\n", file, line);
        exit(255);
    }
}
