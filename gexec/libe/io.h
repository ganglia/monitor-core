/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __IO_H
#define __IO_H

int io_write_bytes(int fd, void *send_buf, int nbytes);
int io_read_bytes(int fd, void *recv_buf, int nbytes);
int io_slurp_file(char *pathname, char **buf, int *len);

#endif /* __IO_H */
