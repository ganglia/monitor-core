#include <string.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#include "lib/interface.h"
#include "lib/ganglia.h"
#include "lib/hash.h"
#include "libunp/unp.h"

#include "gmond/metric_typedefs.h"
#include "gmond/node_data_t.h"

#include "cmdline.h"

extern int optopt;
extern int optind;
extern char *optarg;
extern int opterr;

datum_t type;
datum_t name;
datum_t value;
datum_t units;
datum_t slope;

struct gengetopt_args_info args_info;

int main ( int argc, char **argv )
{
   int rval, len;
   XDR xhandle;
   char mcast_data[4096]; 
   int msg_socket;
   uint32_t key = 0; /* user-defined */
   char empty[] = "\0";
   unsigned int slope;
   char *msg_channel;
   char *msg_port;

   if (cmdline_parser (argc, argv, &args_info) != 0)
      exit(1) ;

   if (!( !strcmp("string", args_info.type_arg)
       || !strcmp("int8", args_info.type_arg)
       || !strcmp("uint8", args_info.type_arg)
       || !strcmp("int16", args_info.type_arg)
       || !strcmp("uint16", args_info.type_arg)
       || !strcmp("int32", args_info.type_arg)
       || !strcmp("uint32", args_info.type_arg)
       || !strcmp("float", args_info.type_arg)
       || !strcmp("double", args_info.type_arg)
       || !strcmp("timestamp", args_info.type_arg)))
      {
         fprintf(stderr,"\nInvalid type: %s\n\n", args_info.type_arg);
         fprintf(stderr,"Run %s --help\n\n", argv[0]);
         exit(1);
      }  
 
   if(! strcmp("zero", args_info.slope_arg ))
      {
         slope = 0;
      }
   else if( !strcmp("positive", args_info.slope_arg) )
      {
         slope = 1;
      }
   else if( !strcmp("negative", args_info.slope_arg) )
      { 
         slope = 2;
      }
   else if( !strcmp("both", args_info.slope_arg) )
      {
         slope = 3;
      }
   else
      {
         fprintf(stderr,"\nInvalid slope: %s\n\n", args_info.slope_arg);
         fprintf(stderr,"Run %s --help\n\n", argv[0]);
         exit(1);
      }

   if( args_info.tmax_arg <= 0 )
      {
         fprintf(stderr,"\nTMAX must be greater than zero\n\n");
         fprintf(stderr,"Run %s --help\n\n", argv[0]);
         exit(1);
      }

   if( args_info.dmax_arg < 0 )
      {
         fprintf(stderr,"\nDMAX must be greater than zero\n\n");
         fprintf(stderr,"Run %s --help\n\n", argv[0]);
         exit(1);
      }

   /* Check if the person explicitly stated "mcast_channel" */
   if( args_info.mcast_channel_given )
     {
       msg_channel = args_info.mcast_channel_arg;
     }
   else
     {
       msg_channel = args_info.msg_channel_arg;
     }
     
   /* Check if the person explicitly stated "mcast_port" */
   if( args_info.mcast_port_given )
     {
       msg_port = args_info.mcast_port_arg;
     }
   else
     {
       msg_port = args_info.msg_port_arg;
     }

   /* Create a UDP socket */
   msg_socket = Udp_connect( msg_channel, msg_port );

   /* Check if the person explicitly stated "mcast_ttl" */
   if( args_info.mcast_ttl_given )
     {
       Mcast_set_ttl( msg_socket, args_info.mcast_ttl_arg );
     }

   if( args_info.mcast_if_given )
     {
       Mcast_set_if( msg_socket, args_info.mcast_if_arg, 0 );
     }
 
   name.data = args_info.name_arg;
   name.size = strlen( name.data )+1;
      if (strcspn(name.data," \t") == 0)
              err_quit("gmetric: --name (-n) can't be blank or empty");

   value.data = args_info.value_arg;
   value.size = strlen( value.data )+1;
   type.data = args_info.type_arg;
   type.size = strlen(type.data)+1;
   if( args_info.units_given )
      {
         units.data = args_info.units_arg;
         units.size = strlen( units.data )+1;
      }
   else
      {
         units.data = empty;
         units.size = 1;
      }

   xdrmem_create(&xhandle, mcast_data, MAX_MCAST_MSG, XDR_ENCODE);

   rval = xdr_u_int(&xhandle, &key);
   if ( rval == 0 )
      {
         err_ret("xdr_u_int for key failed");
         return SYNAPSE_FAILURE;
      }

   rval = xdr_bytes(&xhandle, (char **)&(type.data), &(type.size), MAX_TYPE_LEN);
   if ( rval == 0 )
      {
         err_ret("xdr_bytes() for type failed");
         return SYNAPSE_FAILURE;
      }
   rval = xdr_bytes(&xhandle, (char **)&(name.data),  &(name.size),  MAX_MCAST_MSG);
   if ( rval == 0 )
      {
         err_ret("xdr_bytes() for name failed");
         return SYNAPSE_FAILURE;
      }
   rval = xdr_bytes(&xhandle, (char **)&(value.data), &(value.size), FRAMESIZE);
   if ( rval == 0 )
      {
         err_ret("xdr_bytes() for metric value failed. Perhaps your value is longer than the max of %d characters", FRAMESIZE);
         return SYNAPSE_FAILURE;
      }
   rval = xdr_bytes(&xhandle, (char **)&(units.data), &(units.size), MAX_UNITS_LEN);
   if ( rval == 0 )
      {
         err_ret("xdr_bytes() for units failed");
         return SYNAPSE_FAILURE;
      }

   rval = xdr_u_int(&xhandle, &slope);
   if ( rval == 0 )
      {
         err_ret("xdr_u_int for slope failed");
         return SYNAPSE_FAILURE;
      }

   rval = xdr_u_int(&xhandle, &(args_info.tmax_arg));
   if ( rval == 0 )
      {
         err_ret("xdr_u_int for tmax failed");
         return SYNAPSE_FAILURE;
      }
   rval = xdr_u_int(&xhandle, &(args_info.dmax_arg));
   if ( rval == 0 )
      {
         err_ret("xdr_u_int for dmax failed");
         return SYNAPSE_FAILURE;
      }

   len = xdr_getpos(&xhandle);

   rval = writen( msg_socket, mcast_data, len );
   if ( rval <0)
      {
         err_ret("unable to send data on multicast channel");
         return SYNAPSE_FAILURE;
      }

   return 0;
}
