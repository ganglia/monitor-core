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

extern int RRD_update( char *rrd, char *value );
extern int RRD_create( char *rrd, char *polling_interval);

extern unsigned int metric_hash (char *, unsigned int);
extern const char *in_metric_list (char *, unsigned int);
extern struct ganglia_metric metrics[];

typedef struct
   {
      int rval;
      unsigned int index;
      val_t sum[MAX_HASH_VALUE];
      unsigned int num[MAX_HASH_VALUE];
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
  unsigned int len, hash_val;

  if(! strcmp("METRIC", el) )
     {
        tn          = 0;
        tmax        = 0;
        is_volatile = 0;
        is_numeric  = 0;

        for(i = 0; attr[i] ; i+=2)
           {
              if(! strcmp( attr[i], "NAME"))
                 {
                    xml_data->metric = (char *)attr[i+1];
                 }
              else if(! strcmp( attr[i], "VAL"))
                 {
                    xml_data->metric_val = (char *)attr[i+1];
                 }
              else if(! strcmp( attr[i], "TN"))
                 {
                    tn = atoi( attr[i+1] );
                 }
              else if(! strcmp( attr[i], "TMAX"))
                 {
                    tmax = atoi( attr[i+1] );
                 }
              else if(! strcmp( attr[i], "SLOPE"))
                 {
                    /* SLOPE != zero */
                    if( strcmp( attr[i+1], "zero" ))
                       is_volatile = 1; 
                 }
              else if(! strcmp( attr[i], "TYPE"))
                 {
                    /* TYPE != string */
                    if( strcmp( attr[i+1], "string" ))
                       is_numeric = 1;
                 }
           }
 
        /* Only process fresh data, volatile, numeric data */
        if(! ((tn < tmax *4) && is_volatile && is_numeric) )
           return;

        len = strlen ( xml_data->metric );
        if( in_metric_list( xml_data->metric, len) )
           {
              /* We are going to sum all builtin metrics for summary RRDs */
              debug_msg("%s is in the metric hash (builtin)", xml_data->metric);
              hash_val = metric_hash(xml_data->metric, len);
              debug_msg("[%d][%d] will be incremented %s", xml_data->index , hash_val, xml_data->metric_val);

              switch ( metrics[hash_val].type )
                 {
                    case FLOAT:
                       xml_data->sum[hash_val].f +=  strtod( (const char *)(xml_data->metric_val), (char **)NULL);
                       xml_data->num[hash_val]++;
                       debug_msg("sum = %f num = %d", xml_data->sum[hash_val].f, xml_data->num[hash_val]++ );
                       break;
                    case UINT32:
                       xml_data->sum[hash_val].uint32 += strtoul(xml_data->metric_val, (char **)NULL, 10);
                       xml_data->num[hash_val]++;
                       debug_msg("sum = %ld num = %d", xml_data->sum[hash_val].uint32, xml_data->num[hash_val]++); 
                       break;
                    case DOUBLE:
                       xml_data->sum[hash_val].d = strtod( (const char *)(xml_data->metric_val), (char **)NULL) ;
                       xml_data->num[hash_val]++;
                       debug_msg("sum = %f num = %d", xml_data->sum[hash_val].d, xml_data->num[hash_val]++);
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
     }
  else if(! strcmp("HOST", el) )
     {
        for(i = 0; attr[i] ; i+=2)
           {
              if(! strcmp( attr[i], "NAME" ))
                 {
                    xml_data->host = realloc( xml_data->host, strlen(attr[i+1])+1 );
                    strcpy( xml_data->host, attr[i+1] ); 
                 }
           }

     }
  else if(! strcmp("CLUSTER", el) )
     {
        /* Flush the sums */
        memset( xml_data->sum, 0, MAX_HASH_VALUE);

        /* Flush the nums */
        memset( xml_data->num, 0, MAX_HASH_VALUE);

        for(i = 0; attr[i] ; i+=2)
           {
              if(! strcmp( attr[i], "NAME" ))
                 {
                    xml_data->cluster = realloc ( xml_data->cluster, strlen(attr[i+1])+1 );
                    strcpy( xml_data->cluster, attr[i+1] );
                 }
           }
     }

  return;
}

static void
end (void *data, const char *el)
{
  xml_data_t *xml_data = (xml_data_t *)data;
  if(! strcmp("CLUSTER", el) )
     {

     }
  return;
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
