/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <e/e_error.h>
#include <e/net.h>
#include "auth.h"

void auth_init_credentials(credentials *creds, int lifetime)
{
    time_t now;
    
    time(&now);
    creds->uid        = getuid();
    creds->gid        = getgid();
    creds->valid_from = now;
    creds->valid_to   = now + lifetime;
}

int auth_get_signature(credentials *creds, signature *creds_sig)
{
    int     sock;
    char    auth_sock_path[UNIX_PATH_MAX];
    
    sprintf(auth_sock_path, AUTH_SOCK_PATH, getpid());
    if (net_cli_unixsock_create(&sock, auth_sock_path,
                                AUTHD_SOCK_PATH) != E_OK)
        return AUTH_NET_ERROR;
    if (net_send_bytes(sock, creds, sizeof(credentials)) != E_OK) {
        close(sock);
        return AUTH_NET_ERROR;
    }
    if (net_recv_bytes(sock, creds_sig, sizeof(signature)) != E_OK) {
        close(sock);
        return AUTH_NET_ERROR;
    }
    close(sock);
    return AUTH_OK;
}

int auth_verify_signature(credentials *creds, signature *creds_sig)
{
    int     error;
    RSA     *pub_key = NULL;
    FILE    *fp = NULL;

    if ((fp = fopen(AUTH_PUB_KEY, "r")) == NULL) {
        error = AUTH_FOPEN_ERROR;
        goto cleanup;
    }
    if ((pub_key = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL)) == NULL) {
        error = AUTH_RSA_ERROR;
        goto cleanup;
    }
    ERR_load_crypto_strings();
    if (RSA_verify(0, (unsigned char *)creds, sizeof(credentials), 
                   creds_sig->data, AUTH_RSA_SIGLEN, pub_key) == 0) {
        error = AUTH_RSA_ERROR;
        goto cleanup;
    }
    RSA_free(pub_key);
    fclose(fp);
    return AUTH_OK;

 cleanup:    
    if (pub_key != NULL)
        RSA_free(pub_key);
    if (fp != NULL) 
        fclose(fp);
    return error;
}
