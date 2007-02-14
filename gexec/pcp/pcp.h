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
