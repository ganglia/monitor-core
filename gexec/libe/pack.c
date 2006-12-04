/*
 * Adapted using code from:
 * 
 *   Internet Indirection Infrastructure (i3) Project
 *   University of California at Berkeley
 *   Computer Science Division
 *
 *   http://i3.cs.berkeley.edu
 */
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/types.h>
#include "e_error.h"
#include "pack.h"

/* pack: pack binary items into buf, return length */
int pack(u_char *buf, char *fmt, ...)
{
    va_list args;
    char    *p;
    u_char   *bp;
    u_short  s;
    u_long   l;

    bp = buf;
    va_start(args, fmt);
    for (p = fmt; *p != '\0'; p++) {
        switch (*p) {
        case 'c':   /* char */
            *bp++ = va_arg(args, int);
            break;
        case 's':   /* short */
            s = va_arg(args, int);
            s = htons(s);
            memmove(bp, (char *)&s, sizeof(u_short));
            bp += sizeof(u_short);
            break;
        case 'l':   /* long */
            l = va_arg(args, u_long);
            l = htonl(l);
            memmove(bp, (char *)&l, sizeof(u_long));
            bp += sizeof(u_long);
            break;
        default:   /* illegal type character */
            va_end(args);
            return E_PACK_FORMAT_ERROR;
        }
    }
    va_end(args);
    return bp - buf;
}

/* unpack: unpack binary items from buf, return length */
int unpack(u_char *buf, char *fmt, ...)
{
    va_list args;
    char    *p;
    u_char   *bp, *pc;
    u_short  *ps;
    u_long   *pl;
  
    bp = buf;  
    va_start(args, fmt);
    for (p = fmt; *p != '\0'; p++) {
        switch (*p) {
        case 'c':   /* char */
            pc = va_arg(args, u_char*);
            *pc = *bp++;
            break;
        case 's':   /* short */
            ps = va_arg(args, u_short*);
            *ps = ntohs(*(u_short*)bp);
            bp += sizeof(u_short);
            break;
        case 'l':   /* long */
            pl = va_arg(args, u_long*);
            *pl = ntohl(*(u_long*)bp);
            bp += sizeof(u_long);
            break;
        default:   /* illegal type character */
            va_end(args);
            return E_PACK_FORMAT_ERROR;
        }
    }
    va_end(args);
    return bp - buf;
}
