/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ganglia/hash.h>
#include <ganglia/xmlparse.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gmetad.h>

extern char *rrd_rootdir;

int push_data_to_rrd( char *cluster, char *host, char *metric, char *value);
int push_data_to_summary_rrd( char *cluster, char *metric, char *sum, char *num);

extern int RRD_update( char *rrd, char *value );
extern int RRD_create( char *rrd, char *polling_interval);
extern int summary_RRD_create( char *rrd, char *polling_interval);
extern int summary_RRD_update( char *rrd, char *sum, char *num );

extern struct xml_tag *in_xml_list (char *, unsigned int);
extern struct ganglia_metric *in_metric_list (char *, unsigned int);
extern struct ganglia_metric metrics[];

typedef struct
   {
      int rval;
      unsigned int index;
      val_t sum[MAX_METRIC_HASH_VALUE];
      unsigned int num[MAX_METRIC_HASH_VALUE];
      char *cluster;
      char *host;
      char *metric;
      char *metric_val;
   }
xml_data_t;

static void
start (void *data, const char *el, const char **attr)
{
  xml_data_t *xml_data = (xml_data_t *)data;
  int i, tn, tmax, index, is_volatile, is_numeric;
  struct ganglia_metric *gm;
  struct xml_tag *xt;

  if(! (xt = in_xml_list ( el, strlen( el ))) )
     return;

  switch( xt->tag )
     {

        case METRIC_TAG:

           tn          = 0;
           tmax        = 0;
           is_volatile = 0;
           is_numeric  = 0;

           for(i = 0; attr[i] ; i+=2)
              {
                 if(!( xt = in_xml_list ( attr[i], strlen(attr[i]))) )
                    continue;

                 switch( xt->tag )
                    {

                       case NAME_TAG:
                          xml_data->metric = (char *)attr[i+1];
                          break;
                       
                       case VAL_TAG:
                          xml_data->metric_val = (char *)attr[i+1];
                          break;
 
                       case TN_TAG:
                          tn = atoi( attr[i+1] );
                          break;

                       case TMAX_TAG:
                          tmax = atoi( attr[i+1] );
                          break;
 
                       case SLOPE_TAG:
                          /* SLOPE != zero */
                          if( strcmp( attr[i+1], "zero" ))
                             is_volatile = 1; 
                          break;

                       case TYPE_TAG:
                          /* TYPE != string */
                          if( strcmp( attr[i+1], "string" ))
                             is_numeric = 1;
                          break;

                    }
              }
 
           /* Only process fresh data, volatile, numeric data */
           if(! ((tn < tmax *4) && is_volatile && is_numeric) )
              return;

           if( gm = in_metric_list( xml_data->metric, strlen(xml_data->metric)) )
              {
                 unsigned int hash_val;

                 /* We are going to sum all builtin metrics for summary RRDs */
                 debug_msg("%s is in the metric hash (builtin)", xml_data->metric);

                 hash_val = metric_hash( xml_data->metric, strlen(xml_data->metric) );

                 switch ( gm->type )
                    {
                       case FLOAT:
                          xml_data->sum[hash_val].f +=  strtod( (const char *)(xml_data->metric_val), (char **)NULL);
                          xml_data->num[hash_val]++;
                          debug_msg("sum = %f num = %d", xml_data->sum[hash_val].f, xml_data->num[hash_val] );
                          break;
                       case UINT32:
                          xml_data->sum[hash_val].uint32 += strtoul(xml_data->metric_val, (char **)NULL, 10);
                          xml_data->num[hash_val]++;
                          debug_msg("sum = %ld num = %d", xml_data->sum[hash_val].uint32, xml_data->num[hash_val]); 
                          break;
                       case DOUBLE:
                          xml_data->sum[hash_val].d = strtod( (const char *)(xml_data->metric_val), (char **)NULL) ;
                          xml_data->num[hash_val]++;
                          debug_msg("sum = %f num = %d", xml_data->sum[hash_val].d, xml_data->num[hash_val]);
                          break;
                    }
              }

           /* Save the data to a round robin database */
           if( push_data_to_rrd( xml_data->cluster, xml_data->host, xml_data->metric, xml_data->metric_val) )
              {
                 /* Pass the error on to save_to_rrd */
                 xml_data->rval = 1;
                 return;
              }
           break;

        case HOST_TAG:

           for(i = 0; attr[i] ; i+=2)
              {
                 if(! strcmp( attr[i], "NAME" ))
                    {
                       xml_data->host = realloc( xml_data->host, strlen(attr[i+1])+1 );
                       strcpy( xml_data->host, attr[i+1] ); 
                    }
              }
           break;


        case CLUSTER_TAG:

           /* Flush the sums */
           memset( xml_data->sum, 0, MAX_METRIC_HASH_VALUE);

           /* Flush the nums */
           memset( xml_data->num, 0, MAX_METRIC_HASH_VALUE);

           for(i = 0; attr[i] ; i+=2)
              {
                 if(! strcmp( attr[i], "NAME" ))
                    {
                       xml_data->cluster = realloc ( xml_data->cluster, strlen(attr[i+1])+1 );
                       strcpy( xml_data->cluster, attr[i+1] );
                    }
              }
           break;

        }  /* end switch */

  return;
}

static void
end (void *data, const char *el)
{
  xml_data_t *xml_data = (xml_data_t *)data;
  register int i;
  struct ganglia_metric *gm;
  int len;
  char sum[64];
  char num[64];

  if(! strcmp("CLUSTER", el) )
     {
        for ( i = 0; i < MAX_METRIC_HASH_VALUE; i++ )
           {
              len = strlen(metrics[i].name);
              if( len )
                 {
                     gm  =  (struct ganglia_metric *)in_metric_list ((const char *)metrics[i].name, len);

                     /* Skip it if we have no hosts reporting the data */
                     if (! xml_data->num[i] )
                        continue;

                     switch ( gm->type )
                        {
                           case FLOAT:
                              sprintf( sum, "%f", xml_data->sum[i].f);
                              sprintf( num, "%d", xml_data->num[i] );
                              break;
                           case DOUBLE:
                              sprintf( sum, "%f", xml_data->sum[i].d); 
                              sprintf( num, "%d", xml_data->num[i] ); 
                              break;
                           case UINT32:
                              sprintf( sum, "%d", xml_data->sum[i].uint32);
                              sprintf( num, "%d", xml_data->num[i] );
                              break;
                        }

                     /* Save the data to a round robin database */
                     push_data_to_summary_rrd( xml_data->cluster, metrics[i].name, sum, num);

                     debug_msg("SAVE SUMMARY INFORMATION %s sum=%s num=%s", metrics[i].name, sum, num);
                 }
           }
     }
  return;
}

int
push_data_to_summary_rrd( char *cluster, char *metric, char *sum, char *num)
{
   int rval;
   char rrd[2024];
   char *polling_interval = "15"; /* secs .. constant for now */
   struct stat st;

   snprintf(rrd, 2024,"%s/%s_%s.rrd", rrd_rootdir, cluster, metric);

   if( stat(rrd, &st) )
      {
         rval = summary_RRD_create( rrd, polling_interval );
         if( rval )
            return rval;
      }
   return summary_RRD_update( rrd, sum, num );
}

int
push_data_to_rrd( char *cluster, char *host, char *metric, char *value)
{
  int rval;
  char rrd[2024];
  char *polling_interval = "15"; /* secs .. constant for now */
  struct stat st;

  snprintf(rrd, 2024,"%s/%s_%s_%s.rrd", rrd_rootdir, cluster, host, metric);

  if( stat(rrd, &st) )
     {
        rval = RRD_create( rrd, polling_interval );
        if( rval )
           return rval;
     } 

  return RRD_update( rrd, value );
}

int
process_xml(int index, char *name, char *buf)
{
   int rval;
   XML_Parser xml_parser;
   xml_data_t xml_data;

   memset( &xml_data, 0, sizeof( xml_data ));
   xml_data.index = index;

   xml_parser = XML_ParserCreate (NULL);
   if (! xml_parser)
      {
         err_msg("save_to_rrd() unable to create XML parser");
         return 1;
      }

   XML_SetElementHandler (xml_parser, start, end);
   XML_SetUserData       (xml_parser, &xml_data);

   rval = XML_Parse( xml_parser, buf, strlen(buf), 1 );
   if(! rval )
      {
         err_msg ("save_to_rrd() XML_ParseBuffer() error at line %d:\n%s\n",
                         XML_GetCurrentLineNumber (xml_parser),
                         XML_ErrorString (XML_GetErrorCode (xml_parser)));
         return 1;
      }

   XML_ParserFree(xml_parser);
   return xml_data.rval;
}
