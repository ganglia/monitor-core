#ifndef __PACK_H
#define __PACK_H

/* Programmer checks that buf has enough data to meet fmt */
int pack(unsigned char *buf, char *fmt, ...);

/* Programmer checks that buf has enough space to meet fmt */
int unpack(unsigned char *buf, char *fmt, ...);

#endif /* __PACK_H */
