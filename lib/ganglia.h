/**
 * @file ganglia.h Main Ganglia Cluster Toolkit Library Header File
 */

#ifndef GANGLIA_H
#define GANGLIA_H 1

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define GANGLIA_DEFAULT_MCAST_CHANNEL "239.2.11.71"
#define GANGLIA_DEFAULT_MCAST_PORT 8649
#define GANGLIA_DEFAULT_XML_PORT 8649

#ifndef SYNAPSE_FAILURE
#define SYNAPSE_FAILURE -1
#endif
#ifndef SYNAPSE_SUCCESS
#define SYNAPSE_SUCCESS 0
#endif


/* 
 * Max multicast message: 1500 bytes (Ethernet max frame size)
 * minus 20 bytes for IPv4 header, minus 8 bytes for UDP header.
 */
#ifndef MAX_MCAST_MSG
#define MAX_MCAST_MSG 1472
#endif

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#endif


/* In order for C++ programs to be able to use my C library */
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif

#if 0
BEGIN_C_DECLS
int ganglia_cluster_init(cluster_t *cluster, char *name, unsigned long num_nodes_in_cluster);
int ganglia_add_datasource(cluster_t *cluster, char *group_name, char *ip, unsigned short port, int opts);
int ganglia_cluster_print(cluster_t *cluster);
END_C_DECLS
#endif

#endif
