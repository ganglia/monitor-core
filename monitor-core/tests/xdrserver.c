#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gm_protocol.h"

#define MAXMSGLEN 1500

int main( void )
{
  XDR x;
  int i, rval, decoded;
  char receive[MAXMSGLEN];
  gangliaMessage message;

  rval = read( 0, receive, MAXMSGLEN );
  if(rval <0)
    {
      fprintf(stderr,"ERROR! Server read error\n");
    }
  fprintf(stderr,"SERVER received %d bytes of raw data\n", rval);

  /* Create the XDR receive stream */
  xdrmem_create(&x, receive, MAXMSGLEN, XDR_DECODE);

  /* Flush the data in the (last) received gangliaMessage */
  memset( &message, 0, sizeof(gangliaMessage));

  /* Read the gangliaMessage from the stream */
  xdr_gangliaMessage(&x, &message);

  /* Find out how much data I decoded */
  decoded = xdr_getpos(&x);
  fprintf(stderr,"SERVER decoded the XDR size=%d\n", decoded);

  return 0;
}
