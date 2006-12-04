#ifndef __ANET_H
#define __ANET_H

int  anet_cli_tcpsock_create(int *sock);
void anet_cli_tcpsock_destroy(int sock);
int  anet_svr_tcpsock_create(int *sock, int svr_port);
void anet_svr_tcpsock_destroy(int sock);

#endif /* __ANET_H */
