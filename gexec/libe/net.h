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
#ifndef __NET_H
#define __NET_H

/* Max backlog length for incoming connections */
#define NET_DEFAULT_BACKLOG   16

/* Taken from /usr/include/sys/un.h, where it is hardcoded */
#define UNIX_PATH_MAX        108

/* Max length of www.xxx.yyy.zzz */
#define IP_STRLEN             16  

int  net_cli_sock_create(int *sock, char *svr_host, int svr_port);
void net_cli_sock_destroy(int sock);
int  net_svr_sock_create(int *sock, int svr_port);
void net_svr_sock_destroy(int sock);
int  net_cli_unixsock_create(int *sock, char *cli_path, char *svr_path);
void net_cli_unixsock_destroy(int sock);
int  net_svr_unixsock_create(int *sock, char *svr_path);
void net_svr_unixsock_destroy(int sock);
int  net_send_bytes(int sock, void *send_buf, int nbytes);
int  net_recv_bytes(int sock, void *recv_buf, int nbytes);
int  net_hosttoip(char *host, char *ip);
int  net_iptohost(char *ip, char *host, int len);
int  net_gethostip(char *ip); 

#endif /* __NET_H */
