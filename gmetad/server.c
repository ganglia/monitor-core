#include <pthread.h>
#include <ganglia/net.h>
#include <ganglia/llist.h>
#include <ganglia/hash.h>
#include "dtd.h"
#include "gmetad.h"

extern g_tcp_socket *   server_socket;
extern pthread_mutex_t  server_socket_mutex;

extern llist_entry *trusted_hosts;

extern hash_t *xml;
extern char *gridname;
extern char *authority;

int
xml_print_func( datum_t *key, datum_t *val, void *arg )
{
   int rval;
   int clifd = *(int *)arg;

   SYS_CALL( rval, write( clifd, val->data, val->size ) );
   if(rval< 0)
      return 1;
   return 0;
}

void *
server_thread ( void *arg)
{
   int  clifd, client_valid, len, rval;
   struct sockaddr_in cliaddr;
   char remote_ip[16];
   llist_entry *le;
   char ganglia_xml[256];

   sprintf(ganglia_xml, "<GANGLIA_XML VERSION=\"%s\" SOURCE=\"gmetad\">\n", VERSION);
   sprintf(ganglia_xml, "%s<GRID NAME=\"%s\" AUTHORITY=\"%s\">\n", ganglia_xml, gridname, authority);
   for (;;)
      {
         client_valid = 0;
         len = sizeof(cliaddr);

         pthread_mutex_lock  ( &server_socket_mutex );
         SYS_CALL( clifd, accept(server_socket->sockfd, (struct sockaddr *)&(cliaddr), &len));
         pthread_mutex_unlock( &server_socket_mutex );
         if ( clifd < 0 )
            {
               err_ret("server_thread() error");
               debug_msg("server_thread() %d clientfd = %d errno=%d\n", pthread_self(), clifd, errno);
               continue;
            }

         my_inet_ntop( AF_INET, (void *)&(cliaddr.sin_addr), remote_ip, 16 ); 

         if( !strcmp(remote_ip, "127.0.0.1") )
            {
               client_valid = 1;
            }
         else
            {
               if( llist_search(&(trusted_hosts), (void *)remote_ip, strcmp, &le) == 0)
                  client_valid = 1;
            }

         if(! client_valid )
            {
               debug_msg("server_thread() %s tried to connect and is not a trusted host");
               close( clifd );
               continue;
            }

         SYS_CALL( rval, write( clifd, DTD, strlen(DTD) ) );
         if(rval < 0)
            {
               debug_msg("server_thread() %d unable to write DTD", pthread_self() );
               close(clifd);
               continue;
            }

         SYS_CALL( rval, write( clifd, ganglia_xml, strlen(ganglia_xml) ));
         if(rval < 0)
            {
               debug_msg("server_thread() %d unable to write GANGLIA_XML", pthread_self() );
               close(clifd);
               continue;
            }
 
         if( hash_foreach (xml, xml_print_func, (void *)&clifd) )
            {
               debug_msg("server_thread() %d unable to write cluster info", pthread_self() );
               close(clifd);
               continue;
            }

         SYS_CALL( rval, write( clifd, "</GRID>\n</GANGLIA_XML>\n", 23));
         if(rval < 0)
            {
               debug_msg("server_thread() %d unable to write </GANGLIA_XML>", pthread_self() );
            }

         close(clifd);        
      }
}
