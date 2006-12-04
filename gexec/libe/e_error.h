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
#ifndef __E_ERROR_H
#define __E_ERROR_H

#include <stdio.h>

#define E_OK                     0
#define E_MALLOC_ERROR          -1
#define E_READ_ERROR            -2
#define E_WRITE_ERROR           -3
#define E_SOCKET_ERROR          -4

#define E_GETHOSTBYADDR_ERROR   -5
#define E_GETHOSTBYNAME_ERROR   -6
#define E_CONNECT_ERROR         -7
#define E_BIND_ERROR            -8
#define E_LISTEN_ERROR          -9

#define E_GETHOSTNAME_ERROR    -10
#define E_GRAPH_CYCLE_ERROR    -11
#define E_MEMLEN_ERROR         -12
#define E_OPEN_ERROR           -13
#define E_STAT_ERROR           -14

#define E_PACK_FORMAT_ERROR    -15 

#define fatal_error(__str)                             \
        fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
        fprintf(stderr, __str);

#define fatal_error_long(__str, __file, __line)    \
        fprintf(stderr, "%s:%d ", __file, __line); \
        fprintf(stderr, __str);

#define e_assert(__expression) \
       _e_assert(__expression, __FILE__, __LINE__)

void _e_assert(int expression, char *file, int line);

#endif /* __E_ERROR_H */
