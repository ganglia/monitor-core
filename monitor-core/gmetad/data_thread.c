/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <string.h>
#include <zlib.h>

#include "gmetad.h"
#include "lib/tpool.h"

extern g3_thread_pool data_source_pool;

extern int debug_level;

extern hash_t *xml;

extern hash_t *root;

extern int process_xml(data_source_list_t *, char *);

void
data_thread ( void *arg )
{
   int i, sleep_time, bytes_read, rval, sock = -1;
   data_source_list_t *d = (data_source_list_t *)arg;
   unsigned int read_index;
   struct timeval start, end;
   gzFile z = NULL;

   if(debug_level)
      {
         fprintf(stderr,"Data thread %d is monitoring [%s] data source\n", (int) pthread_self(), d->name);
         for(i = 0; i < d->num_sources; i++)
            {
               fprintf(stderr, "\t%s : %s\n", d->names[i], d->ports[i]);
            }
      }

   /* Assume the best from the beginning */
   d->dead = 0;

   gettimeofday(&start, NULL);
 
   /* Find the first viable source in list */
   sock = -1;
   for(i=0; i < d->num_sources; i++)
     {
       sock = tcp_connect( d->names[i], d->ports[i]);
       if(! (sock < 0) )
         break; /* success */
     }

   if(sock < 0)
     {
       err_msg("data_thread() got no answer from any [%s] datasource", d->name);
       d->dead = 1;
       goto take_a_break;
     }

    /* Create a zlib stream */
    z = gzdopen( sock, "rb" ); 
    if(!z)
      {
        err_msg("unable to create zlib stream\n");
        goto take_a_break;
      }

    read_index = 0;
    for(;;)
      {
        /* Timeout set to 15 seconds */
        if( readable_timeo( sock, 15 ) <= 0)
          goto take_a_break;
 
        if( (read_index + 1024) > d->len )
          {
            /* We need to malloc more space for the data */
            d->buf = realloc( d->buf, d->len +1024 );
            if(!d->buf)
              {
                err_quit("data_thread() unable to malloc enough room for [%s] XML", d->name);
              }
            d->len +=1024;
          }

        bytes_read = gzread( z, d->buf+read_index, 1023 );

        if (bytes_read < 0)
          {
            err_msg("data_thread() unable to read() socket for [%s] data source", d->name);
            d->dead = 1;
            goto take_a_break;
          }
        else if(bytes_read == 0)
          {
            break; /* We've read all the data */
          }

        read_index+= bytes_read;
      }

    d->buf[read_index] = '\0';

    /* Parse the buffer */
    rval = process_xml(d, d->buf);
    if(rval)
      {
        /* We no longer consider the source dead if its XML parsing
         * had an error - there may be other reasons for this (rrd issues, etc). 
         */
         goto take_a_break;
      }

    /* We processed all the data.  Mark this source as alive */
    d->dead = 0;

  take_a_break:
    if(z)
      {
        gzclose(z);
        z = NULL;
      }
    else
      {
        /* We didn't reach the point where the z stream was created.
           gzclose closes the underlying file descriptor so we need
           to close it ourself. */
        if(sock>0)
          close(sock);
      }

   gettimeofday(&end, NULL);

   /* Sleep somewhere between (step +/- 5sec.) */
   sleep_time = (d->step - 5) + (10 * (rand()/(float)RAND_MAX)) - (end.tv_sec - start.tv_sec);

   if(sleep_time > 0)
     {
       g3_run_later( data_source_pool, data_thread, arg, sleep_time, 0 );
     }
   else
     {
       g3_run( data_source_pool, data_thread, arg);
     }

}
