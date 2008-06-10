/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <e/e_error.h>
#include <e/net.h>
#include "auth.h"
#include "authd.h"
#include "authd_options.h"

static int check_credentials(struct ucred *ucred, credentials *cred)
{
    double lifetime;

    if (ucred->uid != cred->uid || ucred->gid != cred->gid)
        return AUTH_CRED_ERROR;
    lifetime = difftime(cred->valid_to, cred->valid_from);
    if (lifetime > AUTHD_CRED_MAXLIFE)
        return AUTH_CRED_ERROR;
    return AUTH_OK;
}

static int gen_signature(credentials *cred, signature *cred_sig)
{
    int     error;
    int     sig_len;
    RSA     *priv_key = NULL;
    FILE    *fp = NULL;

    if ((fp = fopen(AUTH_PRIV_KEY, "r")) == NULL) {
        error = AUTH_FOPEN_ERROR;
        goto cleanup;
    }
    if ((priv_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL)) == NULL) {
        error = AUTH_RSA_ERROR;
        goto cleanup;
    }
    if (RSA_sign(NID_sha1, (unsigned char *)cred, sizeof(credentials), 
                 cred_sig->data, &sig_len, priv_key) == 0) {
        error = AUTH_RSA_ERROR;
        goto cleanup;
    }
    RSA_free(priv_key);
    e_assert(sig_len == AUTH_RSA_SIGLEN);
    fclose(fp);
    return AUTH_OK;

 cleanup:    
    if (priv_key != NULL)
        RSA_free(priv_key);
    if (fp != NULL) 
        fclose(fp);
    return error;
}

static void *authd_thr(void *arg)
{
    long int     sock = (long int)arg;
    int          len = sizeof(struct ucred); 
    struct ucred ucred;
    credentials  cred;
    signature    cred_sig;

    if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &ucred, &len) < 0)
        goto cleanup;
    if (net_recv_bytes(sock, &cred, sizeof(credentials)) != E_OK)
        goto cleanup;
    if (check_credentials(&ucred, &cred) != AUTH_OK)
        goto cleanup;
    if (gen_signature(&cred, &cred_sig) != AUTH_OK)
        goto cleanup;
    if (net_send_bytes(sock, &cred_sig, sizeof(signature)) != E_OK)
        goto cleanup;
    close(sock);
    return NULL;

 cleanup:
    close(sock);
    return NULL;
}

int main(int argc, char **argv)
{
    authd_options       opts;
    long int            svr_sock, cli_sock;
    pthread_attr_t      attr;
    pthread_t           tid;
    struct sockaddr_un  sockaddr;
    socklen_t           sockaddr_len = sizeof(struct sockaddr_un);

    authd_options_init(&opts);
    authd_options_parse(&opts, argc, argv);
    if (opts.daemon)
        daemon(0, 0);

    signal(SIGPIPE, SIG_IGN);
    if (net_svr_unixsock_create((int *)&svr_sock, AUTHD_SOCK_PATH) != E_OK) {
        fprintf(stderr, "Bind error on Unix domain socket %s\n",
                AUTHD_SOCK_PATH);
        exit(2);
    }

    ERR_load_crypto_strings();
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (;;) {
        if ((cli_sock = accept(svr_sock, (struct sockaddr *)&sockaddr,
                               &sockaddr_len)) < 0) {
            if (errno == EINTR)
                continue;
            fatal_error("accept error\n");
            exit(1);
        }
        if (pthread_create(&tid, &attr, authd_thr, (void *)cli_sock) != 0)
            close(cli_sock);
    }

    pthread_attr_destroy(&attr);
    close(svr_sock);
    return 0;
}
