/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __PCP_LIB_H 
#define __PCP_LIB_H 

#include <pthread.h>
#include <semaphore.h>
#include <openssl/sha.h>
#include <e/barrier.h>
#include <e/tree.h>

/* Command types */
#define PCP_NULL_CMD        -1
#define PCP_WRITE_CMD        0
#define PCP_DELETE_CMD       1
#define PCP_CHECKSUM_CMD     2

/* Command arguments */
typedef struct {
    char    *localfile;
    char    *remotefile;
} pcp_write_cmd;

typedef struct {
    char    *remotefile;    
} pcp_delete_cmd;

typedef struct {
    char    *remotefile;
} pcp_checksum_cmd;

#define PCP_CRED_LIFETIME       30
#define PCP_DEF_FRAG_SIZE    32768
#define PCP_DEF_FANOUT           1
#define PCP_DEF_PORT           2850
#define PCP_NUM_TICKS           50

#define PCP_CHECKSUM_SIZE     SHA_DIGEST_LENGTH

typedef struct {
    int verbose;
    int port;
    int fanout;
    int frag_size;
    int best_effort;
} pcp_cmd_options;

int pcp_write(int nhosts, char **ips, char *localfile, char *remotefile, 
              pcp_cmd_options *opts);
int pcp_delete(int nhosts, char **ips, char *remotefile, pcp_cmd_options *opts);
int pcp_checksum(int nhosts, char **ips, char *remotefile, pcp_cmd_options *opts);

#define PCP_OK                        0

#define PCP_AUTH_ERROR               -1
#define PCP_NET_ERROR                -2
#define PCP_IO_ERROR                 -3
#define PCP_LOCALFILE_ERROR          -4
#define PCP_REMOTEFILE_ERROR         -5

#define PCP_PTHREADS_ERROR           -6
#define PCP_SETUID_ERROR             -7
#define PCP_SETGID_ERROR             -8
#define PCP_INTERNAL_ERROR           -9
#define PCP_REMOTEFILE_PERM_ERROR   -10

#define PCP_CMD_PENDING             -99

typedef struct {
    pthread_t       send_tid;
    pthread_t       recv_tid;

    tree            *tree;
    int             self;
    int             nchildren;
    int             *child_nodes;
    char            **child_ips;

    int             *child_socks;

    int             cmd_type;
    void            *cmd;
    pcp_cmd_options *cmd_opts;
    int             cmd_rval;

    int             ndone;
    sem_t           cleanup_sem;
    sem_t           killme_sem;

} pcp_state;

/* Miscellaneous constants */

#define PCP_CRED_LIFETIME             30

#endif /* __PCP_LIB_H */
