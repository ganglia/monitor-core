/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __TOKEN_ARRAY_H
#define __TOKEN_ARRAY_H

void token_array_create(char ***tokens, int *ntokens, char *str, char delim);
char *token_array_maxtoken(char **tokens, int ntokens);
void token_array_destroy(char **tokens, int ntokens);

#endif /* __TOKEN_ARRAY_H */
