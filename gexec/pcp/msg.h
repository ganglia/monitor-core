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
#ifndef __MSG_H
#define __MSG_H

#include <unistd.h>
#include <sys/types.h>
#include "pcp_lib.h"

/* Message types */
#define PCP_TREE_CREATE_MSG        0
#define PCP_WRITE_MSG              1
#define PCP_DELETE_MSG             2
#define PCP_CHECKSUM_MSG           3

/* Reply message types */
/* (No reply for TREE_CREATE when successful) */ 
#define PCP_WRITE_REPLY_MSG       11
#define PCP_DELETE_REPLY_MSG      12
#define PCP_CHECKSUM_REPLY_MSG    13

/* Error reply message types */
#define PCP_TREE_CREATE_ERROR_MSG 20
#define PCP_WRITE_ERROR_MSG       21
#define PCP_DELETE_ERROR_MSG      22
#define PCP_CHECKSUM_ERROR_MSG    23

/* Heartbeat message types */
#define PCP_HEARTBEAT_MSG         30

/* All nodes in the tree */
#define PCP_DST_ALL             -1

#define PCP_HDR_BUILD(__hdr, __type, __src, __dst, __bdy_len) \
        (__hdr)->type    = __type;    \
        (__hdr)->src     = __src;     \
        (__hdr)->dst     = __dst;     \
        (__hdr)->bdy_len = __bdy_len;

/* Header */
typedef struct {
    int type;
    int src;
    int dst;
    int bdy_len;
} pcp_hdr;

typedef struct {
    int  rval;
} pcp_w_reply_bdy;

typedef struct {
    int rval;
} pcp_d_reply_bdy;

typedef struct {
    int           rval;
    unsigned char checksum[PCP_CHECKSUM_SIZE];
} pcp_c_reply_bdy;

typedef struct {
    int rval;
    int bad_node;
} pcp_tc_error_bdy;

typedef struct {
    int rval;
} pcp_w_error_bdy;

typedef struct {
    int rval;
} pcp_d_error_bdy;

typedef struct {
    int rval;
} pcp_c_error_bdy;

typedef struct {
    int xxx;
} pcp_hb_bdy;

#endif /* __MSG_H */
