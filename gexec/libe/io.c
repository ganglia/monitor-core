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
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "e_error.h"
#include "io.h"

int io_write_bytes(int fd, void *send_buf, int nbytes)
{
    int n, bytes_written;

    bytes_written = 0;
    while (bytes_written != nbytes) {
        if ((n = write(fd, (void *)((long int)send_buf + (long int)bytes_written), 
                       nbytes - bytes_written)) <= 0) {
            /* write is a "slow" syscall, must be manually restarted */
            if (errno == EINTR) 
                continue;
            return E_WRITE_ERROR;
        }
        bytes_written += n;
    }
    return E_OK;
}

int io_read_bytes(int fd, void *recv_buf, int nbytes)
{   
    int n, bytes_read;

    bytes_read = 0;
    while (bytes_read != nbytes) {
        if ((n = read(fd, (void *)((long int)recv_buf + (long int)bytes_read), 
                      nbytes - bytes_read)) <= 0) {
            /* read is a "slow" syscall, must be manually restarted */
            if (errno == EINTR)
                continue;
            return E_READ_ERROR;
        }
        bytes_read += n;
    }
    return E_OK;
}
