#ifndef APR_NET_H
#define APR_NET_H 1

apr_socket_t *
create_udp_client(apr_pool_t *context, char *ipaddr, apr_port_t port);

apr_socket_t *
create_udp_server(apr_pool_t *context, apr_port_t port, char *bind);


#endif
