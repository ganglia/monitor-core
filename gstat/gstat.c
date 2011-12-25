#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "ganglia_gexec.h"
#include "ganglia_priv.h"
#include "llist.h"
#include <getopt.h>
#include "cmdline.h"

char cluster_ip[16];
unsigned short cluster_port;

struct gengetopt_args_info args_info;

void process_opts( int argc, char **argv );

static int debug_level;

int main(int argc, char *argv[])
{
   int rval;
   gexec_cluster_t cluster;
   gexec_host_t *host;
   llist_entry *li;

   debug_level = 1;
   set_debug_msg_level(debug_level);

   if (cmdline_parser (argc, argv, &args_info) != 0)
      exit(1) ;

   rval = gexec_cluster(&cluster, args_info.gmond_ip_arg, args_info.gmond_port_arg );
   if ( rval != 0)
      {
         printf("Unable to get hostlist from %s %d!\n", args_info.gmond_ip_arg, args_info.gmond_port_arg);
         exit(-1);
      }

   if( args_info.mpifile_flag )
      {
         if( args_info.all_flag )
            li = cluster.hosts;
         else
            li = cluster.gexec_hosts;

         for( ; li != NULL; li = li->next )
            {
               host = li->val;
               if( host->name_resolved && ! args_info.numeric_flag )
                  {
                     if(!strcmp(host->domain, "unspecified"))
                        printf("%s:%d\n", host->name, host->cpu_num);
                     else
                        printf("%s.%s:%d\n", host->name, host->domain, host->cpu_num);
                  }
               else
                  {
                     printf("%s:%d\n", host->ip, host->cpu_num);
                  }
            }
         exit(0);
      }

   if(! args_info.list_flag )
      {
         printf("CLUSTER INFORMATION\n");
         printf("       Name: %s\n", cluster.name);
         printf("      Hosts: %d\n", cluster.num_hosts);
         printf("Gexec Hosts: %d\n", cluster.num_gexec_hosts);
         printf(" Dead Hosts: %d\n", cluster.num_dead_hosts);
         printf("  Localtime: %s\n", ctime(&(cluster.localtime)) );
      }

   if( args_info.dead_flag)
      {
         if(! args_info.list_flag )
            printf("DEAD CLUSTER HOSTS\n");
         if(cluster.num_dead_hosts)
            {
               if(! args_info.list_flag )
                  printf("%32.32s   Last Reported\n", "Hostname");
               for( li = cluster.dead_hosts; li != NULL; li = li->next)
                  {
                     host= li->val;
                     printf("%32.32s   %s", host->name, ctime(&(host->last_reported)));
                  }
            }   
         else
            {
               printf("There are no hosts down at this time\n");
            }
         gexec_cluster_free(&cluster);
         exit(0); 
      }

   if( args_info.all_flag )
      {
         li = cluster.hosts;
         if(! cluster.num_hosts )
            {
               printf("There are no hosts up at this time\n"); 
               gexec_cluster_free(&cluster);
               exit(0);
            }
      }
   else
      {
         li = cluster.gexec_hosts;
         if(! cluster.num_gexec_hosts)
            {
               printf("There are no hosts running gexec at this time\n");
               gexec_cluster_free(&cluster);
               exit(0);
            }
      }

   if(! args_info.list_flag )
      {
         printf("CLUSTER HOSTS\n");
         printf("Hostname                     LOAD                       CPU              Gexec\n");
         printf(" CPUs (Procs/Total) [     1,     5, 15min] [  User,  Nice, System, Idle, Wio]\n\n");
      }
   for(; li != NULL; li = li->next)
      {
         host = li->val;
         if( host->name_resolved && ! args_info.numeric_flag )
                  {
                     if(!strcmp(host->domain, "unspecified"))
                        printf("%s", host->name);
                     else
                        printf("%s.%s", host->name, host->domain);
                  }
               else
                  {
                     printf("%s", host->ip);
                  }
         if( args_info.single_line_flag )
            printf(" ");
         else
            printf("\n");

         printf(" %4d (%5d/%5d) [%6.2f,%6.2f,%6.2f] [%6.1f,%6.1f,%6.1f,%6.1f,%6.1f] ",
               host->cpu_num, host->proc_run, host->proc_total,
               host->load_one, host->load_five, host->load_fifteen,
               host->cpu_user, host->cpu_nice, host->cpu_system, host->cpu_idle, host->cpu_wio);
         if(host->gexec_on)
            printf("%s\n", "ON");
         else
            printf("%s\n", "OFF"); 
      }

   gexec_cluster_free(&cluster);
   return 0;
}

