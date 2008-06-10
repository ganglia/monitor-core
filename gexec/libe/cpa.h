/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#ifndef __CPA_H
#define __CPA_H

void cpa_create(char ***dst_array, int *dst_array_size,
                char **src_array, int src_array_size);
int  cpa_create_from_buf(char ***array, int *array_size,
                         void *buf, int buf_len);
void *cpa_malloc_buf(char **array, int array_size, int *buf_len);
void cpa_destroy(char **array, int array_size);

#endif /* __CPA */
