/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#include "gmetad.h"
#include "lib/gzio.h"
#include "lib/tpool.h"

/*
extern ganglia_thread_pool data_source_pool;
*/

extern int debug_level;

extern hash_t *xml;

extern hash_t *root;

extern int process_xml(data_source_list_t *, char *);

extern gmetad_config_t gmetad_config;

void *
data_thread ( void *arg )
{
   int i, sleep_time, rval, sock = -1;
   data_source_list_t *d = (data_source_list_t *)arg;
   struct timeval start, end;
   gzFile gz = NULL;
   char *output, *p;
   int output_index;
   int output_len;
   int count;

   if(debug_level)
      {
         fprintf(stderr,"Data thread %d is monitoring [%s] data source\n", (int) pthread_self(), d->name);
         for(i = 0; i < d->num_sources; i++)
            {
               fprintf(stderr, "\t%s : %s\n", d->names[i], d->ports[i]);
            }
      }

   output = malloc( 2048);
   output_len = 2048;

   for(;;)
     {
       /* Assume the best from the beginning */
       d->dead = 0;
    
       gettimeofday(&start, NULL);
    
       /* Find the first viable source in list */
       sock = -1;
       for(i=0; i < d->num_sources; i++)
         {
           sock = tcp_connect( d->names[i], d->ports[i]);
           if(sock >= 0)
             break; /* success */
         }
    
       if(sock < 0)
         {
           err_msg("data_thread() got no answer from any [%s] datasource", d->name);
           d->dead = 1;
/*
           goto take_a_break;
*/
         }
       else
         {
           d->last_heard_from = time(NULL);
           debug_msg("Successfully connected to [%s]", d->name);

           /* Collect the data from the remote source */
           gz = NULL;
           gz = ganglia_gzdopen( sock, "r" );
           if(!gz)
             {
               err_msg("data_thread() unable to gzdopen socket for [%s]", d->name);
               d->dead = 1;
               goto take_a_break;
             }
 
           output_index = 0;
           for(;;)
             {
               if( (output_len - output_index) < 2048 )
                 {
                   output_len += 2048;
                   output = realloc( output, output_len );
                   assert( output != NULL );
                 } 

               count = ganglia_gzread( gz, output+output_index, 2047 );
               if( count < 0 )
                 {
                   err_msg("gzread error on %s", d->name);
                   d->dead =1;
                   goto take_a_break;
                 }
               if(count == 0)
                 break;

               output_index += count;
             }
           *(output + output_index) = '\0';

           /* This is a hack.  It's ugly.  */
           if( gmetad_config.force_names )
             {
               char *q;
    
               p = strstr(output, "<GANGLIA_XML");
               if(!p)
                 {
                   d->dead = 1;
                   goto take_a_break;
                 }
               while(*p && *p != '>'){ p++ ;}
               p++;
    
               /* So p now points just after the open GANGLIA_XML .. we need to take
                  off the last GANGLIA_XML */
            
               q = strstr(p, "GANGLIA_XML>");
               if(!q)
                 {
                   d->dead = 1;
                   goto take_a_break;
                 }
    
               while(*q && *q != '<'){ q-- ;}
               q--;
               /* q points to just before the start of </GANGLIA_XML> */
               *q = '\0';  /* strip off the GANGLIA_XML altogether */        
             }
           else
             {
               p = output;
             }
         }
    
       /* Parse the buffer */
       rval = process_xml(d, d->dead? NULL: p);
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
       if(gz)
         {
           ganglia_gzclose(gz);
           gz= NULL;
         }
     
       gettimeofday(&end, NULL);
    
       /* Sleep somewhere between (step +/- 5sec.) */
       sleep_time = (d->step - 5) + (10 * (rand()/(float)RAND_MAX)) - (end.tv_sec - start.tv_sec);
   
       if(sleep_time > 0)
         {
           sleep(sleep_time);
         }
    }
}
