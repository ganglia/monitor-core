#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <gmetad.h>
#include <string.h>
#include <zlib.h>

#include <apr_time.h>

/* Deliberately vary the sleep interval by this percentage: */
#define SLEEP_RANDOMIZE 5.0

extern hash_t *xml;

extern hash_t *root;

extern int process_xml(data_source_list_t *, char *);

void *
data_thread ( void *arg )
{
   int i, bytes_read, rval;
   data_source_list_t *d = (data_source_list_t *)arg;
   g_inet_addr *addr;
   g_tcp_socket *sock=0;
   datum_t key;
   char *buf;
   /* This will grow as needed */
   unsigned int buf_size = 1024*128, read_index, read_available;
   struct pollfd struct_poll;
   apr_time_t start, end;
   apr_interval_time_t sleep_time, elapsed;
   double random_factor;
   unsigned int rand_seed;

   rand_seed = apr_time_now() * (int)pthread_self();
   for(i = 0; d->name[i] != 0; rand_seed = rand_seed * d->name[i++]);
 
   if(get_debug_msg_level())
      {
         fprintf(stderr,"Data thread %lu is monitoring [%s] data source\n", (unsigned long)pthread_self(), d->name);
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
         start = apr_time_now();
         sock = NULL;
         
         /* If we successfully read from a good data source last time then try the same host again first. */
         if(d->last_good_index != -1)
           sock = g_tcp6_socket_new ( d->sources[d->last_good_index] );

         /* If there was no good connection last time or the above connect failed then try each host in the list. */
         if(!sock)
           {
             for(i=0; i < d->num_sources; i++)
               {
                 /* Find first viable source in list. */
                 sock = g_tcp6_socket_new ( d->sources[i] );
                 if( sock )
                   {
                     d->last_good_index = i;
                     break;
                   }
                 else
                   {
                     err_msg("data_thread() for [%s] failed to contact node %s", d->name, d->sources[i]->name);
                   }
               }
           }

         if(!sock)
            {
               err_msg("data_thread() got no answer from any [%s] datasource", d->name);
               d->dead = 1;
               goto take_a_break;
            }

         struct_poll.fd = sock->sockfd;
         struct_poll.events = POLLIN; 

         read_index = 0;
         for(;;)
            {
               /* Timeout set to 10 seconds */
               rval = poll( &struct_poll, 1, 10000);
               if( rval < 0 )
                  {
                     /* Error */
                     err_msg("poll() error in data_thread from source %d for [%s] data source after %d bytes read", d->last_good_index, d->name, read_index);
                     if (d->last_good_index < (d->num_sources - 1))
                        d->last_good_index += 1; /* skip this source */
                     else
                        d->last_good_index = -1; /* forget this source */
                     d->dead = 1;
                     goto take_a_break;
                  }
               else if (rval == 0)
                  {
                     /* No revents during timeout period */
                     err_msg("poll() timeout from source %d for [%s] data source after %d bytes read", d->last_good_index, d->name, read_index);
                     if (d->last_good_index < (d->num_sources - 1))
                        d->last_good_index += 1; /* skip this source */
                     else
                        d->last_good_index = -1; /* forget this source */
                     d->dead = 1;
                     goto take_a_break; 
                  }
               else
                  {
                     if( struct_poll.revents & POLLIN )
                        {
                           if(ioctl(sock->sockfd, FIONREAD, &read_available) == -1)
                           {
                                 err_msg("data_thread() unable to ioctl(FIONREAD) socket for [%s] data source", d->name);
                                 d->last_good_index = -1;
                                 d->dead = 1;
                                 goto take_a_break;
                           }
                           if( (read_index + read_available) > buf_size )
                              {
                                 /* We need to malloc more space for the data */
                                 buf = realloc( buf, (buf_size+read_available)*2 );
                                 if(!buf)
                                    {
                                       err_quit("data_thread() unable to malloc enough room for [%s] XML", d->name);
                                    }
                                 buf_size=(buf_size+read_available)*2;
                              }
                           bytes_read = read(sock->sockfd, buf+read_index, read_available);
                           if (bytes_read < 0)
                              {
                                 err_msg("data_thread() unable to read() socket for [%s] data source", d->name);
                                 d->last_good_index = -1;
                                 d->dead = 1;
                                 goto take_a_break;
                              }
                           else if(bytes_read == 0)
                              {
                                 break;
                              }
                           read_index+= bytes_read;
                        }
                        /* Appears that OSX uses POLLHUP on Sockets that I have loaded the entire message into the buffer...
                         * not that I lost the connection (See FreeBSD lists on this discussion)
                         */
#if !(defined(DARWIN))
                     if( struct_poll.revents & POLLHUP )
                        {
                           err_msg("The remote machine closed connection for [%s] data source after %d bytes read", d->name, read_index);
                           d->last_good_index = -1;
                           d->dead = 1;
                           goto take_a_break;
                        }
#endif /* DARWIN */
                     if( struct_poll.revents & POLLERR )
                        {
                           err_msg("POLLERR! for [%s] data source after %d bytes read", d->name, read_index);
                           d->last_good_index = -1;
                           d->dead = 1;
                           goto take_a_break;
                        }
                     if( struct_poll.revents & POLLNVAL )
                        {
                           err_msg("POLLNVAL! for [%s] data source after %d bytes read", d->name, read_index);
                           d->last_good_index = -1;
                           d->dead = 1;
                           goto take_a_break;
                        }
                  }
            }

         /* These are the gzip header magic numbers, per RFC 1952 section 2.3.1 */
         if(read_index > 2 && (unsigned char)buf[0] == 0x1f && (unsigned char)buf[1] == 0x8b)
	   {
	     /* Uncompress the buffer */
	     int ret;
	     z_stream strm;
	     char * uncompressed;
	     unsigned int write_index = 0;

	     if( get_debug_msg_level() > 1 )
	       {
		 err_msg("GZIP compressed data for [%s] data source, %d bytes", d->name, read_index);
	       }

	     uncompressed = malloc(buf_size * 2);
	     if( !uncompressed )
	       {
		 err_quit("data_thread() unable to malloc enough room for [%s] GZIP", d->name);
	       }

	     strm.zalloc = NULL;
	     strm.zfree = NULL;
	     strm.opaque = NULL;
	     strm.next_in  = (Bytef *)buf;
	     strm.avail_in = read_index;

	     /* Initialize the stream, 15 and 16 are magic numbers (gzip and max window size) */
	     ret = inflateInit2(&strm, 15 + 16);
	     if( ret != Z_OK )
	       {
		 err_msg("InflateInitError! for [%s] data source, failed to call inflateInit", d->name);
		 d->dead = 1;

		 free(buf);
		 buf = uncompressed;
		 goto take_a_break;
	       }

	     while (1)
	       {
		 /* Create more buffer space if needed */
		 if ( (write_index * 2) > buf_size)
		   {
		     buf_size *= 2;
		     uncompressed = realloc(uncompressed, buf_size);
		     if(!uncompressed)
		       {
			 err_quit("data_thread() unable to realloc enough room for [%s] GZIP", d->name) ;
		       }
		   }

		 /* Do the inflate */
		 strm.next_out  = (Bytef *)(uncompressed + write_index);
		 strm.avail_out = buf_size - write_index - 1;

		 ret = inflate(&strm, Z_FINISH);
		 write_index = strm.total_out;

		 if (ret == Z_OK || ret == Z_BUF_ERROR)
		   {
		     /* These are normal - just continue on */
		     continue;
		   }
		 else if( ret == Z_STREAM_END )
		   {
		     /* We have finished, set things up for the XML parser */
		     free (buf);
		     buf = uncompressed;
		     read_index = write_index;
		     if(get_debug_msg_level() > 1)
		       {
			 err_msg("Uncompressed to %d bytes", read_index);
		       }
		     break;
		   }
		 else
		   {
		     /* Oh dear, something bad */
		     inflateEnd(&strm);

		     err_msg("InflateError! for [%s] data source, failed to call inflate (%s)", d->name, zError(ret));
		     d->dead = 1;

		     free(buf);
		     buf = uncompressed;
		     goto take_a_break;
		   }
	       }
	     inflateEnd(&strm);
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
         g_tcp_socket_delete(sock);

         end = apr_time_now();
         /* Sleep somewhere between (step +/- SLEEP_RANDOMIZE percent.) */
         random_factor = 1 + (SLEEP_RANDOMIZE / 50.0) * ((rand_r(&rand_seed) - RAND_MAX/2)/(float)RAND_MAX);
         elapsed = end - start;
         sleep_time = apr_time_from_sec(d->step) * random_factor - elapsed;
         if(sleep_time > 0)
           apr_sleep(sleep_time); 
      }
   return NULL;
}
