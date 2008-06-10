/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __PCP_H 
#define __PCP_H 

typedef struct {
    int     nhosts;
    char    **ips;
    char    *localfile;
    char    *remotefile;
} pcp_write_args;

typedef struct {
    int     nhosts;
    char    **ips;
    char    *remotefile;
} pcp_delete_args;

typedef struct {
    int     nhosts;
    char    **ips;
    char    *remotefile;
} pcp_checksum_args;

#endif /* __PCP_H */
