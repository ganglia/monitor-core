/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <gmetad.h>
#include <string.h>
#include <ganglia/hash.h>

extern int debug_level;

extern hash_t *xml;

extern process_xml(data_source_list_t *, char *);

void *
data_thread ( void *arg )
{
   int i, sleep_time, bytes_read, rval;
   data_source_list_t *d = (data_source_list_t *)arg;
   g_inet_addr *addr;
   g_tcp_socket *sock;
   datum_t key, val;
   char *buf;
   /* This will grow as needed */
   unsigned int buf_size = 1024, read_index;
   char *p, *q;
   struct pollfd struct_poll;
 
   if(debug_level)
      {
         fprintf(stderr,"%d is monitoring [%s] data source\n", pthread_self(), d->name);
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
         err_quit("Unable to malloc initial buffer for [%s] data source\n", d->name);
      }

   for (;;)
      {
         for(i=0; i < d->num_sources; i++)
            {
               sock = g_tcp_socket_new ( d->sources[i] );
               if( sock )
                  break;
            }

         if(!sock)
            {
               hash_delete (&key, xml);
               debug_msg("Unable to get any data from [%s] datasource DELETING from XML", d->name);
               goto take_a_break;
            }

         struct_poll.fd = sock->sockfd;
         struct_poll.events = POLLIN; 

         read_index = 0;
         for(;;)
            {
               /* Timeout set to 5 seconds */
               rval = poll( &struct_poll, 1, 5000);
               if( rval < 0 )
                  {
                     /* Error */
                     debug_msg("poll() error in data_thread");
                     hash_delete (&key, xml);
                     goto take_a_break;
                  }
               else if (rval == 0)
                  {
                     /* No revents during timeout period */
                     debug_msg("poll() timeout");
                     hash_delete( &key, xml);
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
                                       err_quit("Unable to malloc enough room for [%s] data source", d->name);
                                    }
                                 buf_size+=1024;
                              }
                           SYS_CALL( bytes_read, read(sock->sockfd, buf+read_index, 1024));
                           if (bytes_read < 0)
                              {
                                 err_msg("Unable to read socket for [%s] data source", d->name);
                                 goto take_a_break;
                              }
                           else if(bytes_read == 0)
                              {
                                 close( sock->sockfd );
                                 break;
                              }
                           
                           read_index+= bytes_read;
                        }
                     if( struct_poll.revents & POLLHUP )
                        {
                           debug_msg("The remote machine closed connection");
                           break;
                        }
                     if( struct_poll.revents & POLLERR )
                        {
                           debug_msg("POLLERR!");
                           break;
                        }
                     if( struct_poll.revents & POLLNVAL )
                        {
                           debug_msg("POLLNVAL!");
                           break;
                        }
                  }
            }

         buf[read_index] = '\0';


         /* Parse the buffer */
         rval = process_xml(d, buf );
         if(rval)
            {
               debug_msg("save_to_rrd() couldn't parse the XML and data to RRD for [%s]", d->name);
               goto take_a_break;
            }    

         p = strstr(buf, "<GANGLIA_XML");
         if(!p)
            {
               err_msg("Unable to find the start of the GANGLIA_XML tag in output from [%s] data source", d->name);
               goto take_a_break;
            }

         p = strchr( p, '>' ); 
         if(!p)
            {
               err_msg("Unable to find end of GANGLIA_XML tag in output from [%s] data source", d->name);
               goto take_a_break;
            }

         p = strchr( p, '\n' );
         if(!p)
            {
               goto take_a_break;
            }
         p++;

         q = strstr(p, "</GANGLIA_XML>");
         if(!q)
            {
               err_msg("Unable to find the closing GANGLIA_XML tag in output from [%s] data source", d->name);
               goto take_a_break;
            } 
         else
            {
               *q = '\0';
            }

         val.data = p;
         val.size = strlen(val.data);

         if(! hash_insert (&key, &val, xml))
            {
               err_msg("Unable to insert data for [%s] into XML hash", d->name);
            }

       take_a_break:
         g_tcp_socket_delete(sock);
         /* 10 to 20 seconds... autoconf later */
         sleep_time=10+(int) (20.0*rand()/(RAND_MAX+10.0));
         sleep(sleep_time);
      }
   

   return NULL;
}
