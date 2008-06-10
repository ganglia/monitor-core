/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __AUTH_H
#define __AUTH_H

#include <sys/types.h>
#include <time.h>

/* Length in bytes of RSA signature */
#define AUTH_RSA_SIGLEN 64

/* Sockname Unix protocol module namespace (add PID) */
#define AUTH_SOCK_PATH  "auth-%d-sock"
/* Sockname in Unix protocol module namespace */
#define AUTHD_SOCK_PATH "authd-sock"

/* NOTE: make configurable in configure script and code */
#define AUTH_PUB_KEY    "/etc/auth_pub.pem"
#define AUTH_PRIV_KEY   "/etc/auth_priv.pem"

typedef struct {
    uid_t   uid;
    gid_t   gid;
    time_t  valid_from;
    time_t  valid_to;
} credentials;

typedef struct {
    unsigned char data[AUTH_RSA_SIGLEN];
} signature;

void auth_init_credentials(credentials *creds, int lifetime);
int  auth_get_signature(credentials *creds, signature *creds_sig);
int  auth_verify_signature(credentials *creds, signature *creds_sig);

#define AUTH_OK             0 
#define AUTH_FOPEN_ERROR   -1
#define AUTH_RSA_ERROR     -2
#define AUTH_CRED_ERROR    -3
#define AUTH_NET_ERROR     -4 

#endif /* __AUTH_H */
