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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "e_error.h"
#include "io.h"
#include "xmalloc.h"

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

/* Caller must free buf */
int io_slurp_file(char *pathname, char **buf, int *len)
{
    int         rval, fd;
    struct stat statbuf;

    fd     = -1;
    (*buf) = NULL;
    (*len) = 0;

    if ((fd = open(pathname, O_RDONLY)) < 0) {
        rval = E_OPEN_ERROR;
        goto cleanup;
    }
    if (fstat(fd, &statbuf) < 0) {
        rval = E_STAT_ERROR;
        goto cleanup;
    }
    (*len) = statbuf.st_size;
    (*buf) = (char *)xmalloc(*len);
    if ((rval = io_read_bytes(fd, *buf, *len)) != E_OK)
        goto cleanup;
    close(fd);
    return E_OK;

 cleanup:
    if ((*buf) != NULL) {
        xfree(*buf);
        (*len) = 0;
        (*buf) = NULL;
    }
    if (fd != -1)
        close(fd);
    return rval;
}
