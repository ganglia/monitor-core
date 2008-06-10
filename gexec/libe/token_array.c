/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#include <stdio.h>
#include <string.h>
#include "bytes.h"
#include "token_array.h"
#include "xmalloc.h"

static char *skip_delims(char *p, char delim, int plen)
{
    int i;
    
    for (i = 0; i < plen; i++)
        if (p[i] != delim)
            return &p[i];
    return NULL;
}

/* Compute ntokens in string (e.g., "  foo   bar baz " has 3 tokens) */
static int compute_ntokens(char *str, char delim)
{
    int  len, plen, ntokens = 0;
    char *p, *q;

    p   = str;
    len = strlen(str);
    do {
        plen = len - (p - str);
        q = skip_delims(p, delim, plen);
        if (q == NULL)
            break;
        ntokens++;
        p = strchr(q, delim);
    } while (p != NULL);
    return ntokens;
}

void token_array_create(char ***tokens, int *ntokens, char *str, char delim)
{
    int  len, plen, qlen, toklen, i = 0;
    char *p, *q, *r;

    *ntokens = compute_ntokens(str, delim);
    *tokens  = (char **)xmalloc((*ntokens) * sizeof(char *));

    p   = str;
    len = strlen(str);
    for (i = 0; i < *ntokens; i++) {
        /* Find token start */
        plen = len - (p - str);
        q = skip_delims(p, delim, plen);
        /* Find token end (may be NULL char) */
        qlen = len - (q - str);
        r = strchr(q, delim);
        /* Compute token len, malloc token, and copy it */
        toklen = (r != NULL) ? (r - q) : qlen;
        (*tokens)[i] = (char *)xmalloc(toklen + NULLBYTE_SIZE);
        memcpy((*tokens)[i], q, toklen);
        (*tokens)[i][toklen] = '\0';
        p = strchr(q, delim);
    }
}

char *token_array_maxtoken(char **tokens, int ntokens)
{
    int i, max = 0;
    
    for (i = 0; i < ntokens; i++)
        if (strlen(tokens[i]) > strlen(tokens[max]))
            max = i;
    return tokens[max];
}

void token_array_destroy(char **tokens, int ntokens)
{
    int i;

    for (i = 0; i < ntokens; i++)
        xfree(tokens[i]);
    xfree(tokens);
}
