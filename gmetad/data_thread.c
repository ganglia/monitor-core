/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <gmetad.h>
#include <string.h>
#include <gmetad.h>
#include <zlib.h>

extern int debug_level;

extern hash_t *xml;

extern hash_t *root;

extern int process_xml(data_source_list_t *, char *);

void *
data_thread ( void *arg )
{
   int i, sleep_time, bytes_read, rval;
   data_source_list_t *d = (data_source_list_t *)arg;
   g_inet_addr *addr;
   g_tcp_socket *sock=0;
   datum_t key;
   char *buf;
   /* This will grow as needed */
   unsigned int buf_size = 1024, read_index;
   struct pollfd struct_poll;
   struct timeval start, end;
   gzFile z = NULL;
 
   if(debug_level)
      {
         fprintf(stderr,"Data thread %d is monitoring [%s] data source\n", (int) pthread_self(), d->name);
         for(i = 0; i < d->num_sources; i++)
            {
               addr = d->sources[i];
               fprintf(stderr, "\t%s\n", addr->name);
            }
      }

   key.data = d->name;
   key.size = strlen( key.data ) + 1;

   buf = malloc( buf_size );
   if(!buf)
      {
         err_quit("data_thread() unable to malloc initial buffer for [%s] data source\n", d->name);
      }

   /* Assume the best from the beginning */
   d->dead = 0;

   for (;;)
      {
         gettimeofday(&start, NULL);
         for(i=0; i < d->num_sources; i++)
            {
               /* Find first viable source in list. */
               sock = g_tcp_socket_new ( d->sources[i] );
               if( sock )
                  break;
            }

         if(!sock)
            {
               err_msg("data_thread() got not answer from any [%s] datasource", d->name);
               d->dead = 1;
               goto take_a_break;
            }

         struct_poll.fd = sock->sockfd;
         struct_poll.events = POLLIN; 
  
         /* Create a zlib stream */
         z = gzdopen( sock->sockfd, "rb" ); 
         if(!z)
           {
             err_msg("unable to create zlib stream\n");
             goto take_a_break;
           }

         read_index = 0;
         for(;;)
            {
               /* Timeout set to 10 seconds */
               rval = poll( &struct_poll, 1, 10000);
               if( rval < 0 )
                  {
                     /* Error */
                     err_msg("poll() error in data_thread");
                     d->dead = 1;
                     goto take_a_break;
                  }
               else if (rval == 0)
                  {
                     /* No revents during timeout period */
                     err_msg("poll() timeout");
                     d->dead = 1;
                     goto take_a_break; 
                  }
               else
                  {
                     if( struct_poll.revents & POLLIN )
                        {
                           if( (read_index + 1024) > buf_size )
                              {
                                 /* We need to malloc more space for the data */
                                 buf = realloc( buf, buf_size+1024 );
                                 if(!buf)
                                    {
                                       err_quit("data_thread() unable to malloc enough room for [%s] XML", d->name);
                                    }
                                 buf_size+=1024;
                              }
                           bytes_read = gzread( z, buf+read_index, 1023 );
                           if (bytes_read < 0)
                              {
                                 err_msg("data_thread() unable to read() socket for [%s] data source", d->name);
                                 d->dead = 1;
                                 goto take_a_break;
                              }
                           else if(bytes_read == 0)
                              {
                                 break;
                              }
                           read_index+= bytes_read;
                        }
                     if( struct_poll.revents & POLLHUP )
                        {
                           err_msg("The remote machine closed connection");
                           d->dead = 1;
                           goto take_a_break;
                        }
                     if( struct_poll.revents & POLLERR )
                        {
                           err_msg("POLLERR!");
                           d->dead = 1;
                           goto take_a_break;
                        }
                     if( struct_poll.revents & POLLNVAL )
                        {
                           err_msg("POLLNVAL!");
                           d->dead = 1;
                           goto take_a_break;
                        }
                  }
            }

         buf[read_index] = '\0';

         /* Parse the buffer */
         rval = process_xml(d, buf);
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
           gzclose(z);
         g_tcp_socket_delete(sock);

         gettimeofday(&end, NULL);
         /* Sleep somewhere between (step +/- 5sec.) */
         sleep_time = (d->step - 5) + (10 * (rand()/(float)RAND_MAX)) - (end.tv_sec - start.tv_sec);
         if( sleep_time > 0 )
            sleep(sleep_time);
      }
   return NULL;
}
