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

char *cluster = NULL;
char *host = NULL;
char *metric = NULL;
char *metric_val = NULL;

extern char *rrd_rootdir;

int push_data_to_rrd( char *cluster, char *host, char *metric, char *value);

extern int RRD_update( char *rrd, char *value );
extern int RRD_create( char *rrd, char *polling_interval);
extern unsigned int metric_hash (char *, unsigned int);
extern const char *in_metric_list (char *, unsigned int);

typedef struct
   {
      int rval;
      unsigned int index;
      double   long sum[MAX_HASH_VALUE];
      unsigned long num[MAX_HASH_VALUE];
   }
sum_num_t;

static void
start (void *data, const char *el, const char **attr)
{
  sum_num_t *sum_num = (sum_num_t *)data;
  int i, is_string, is_constant, is_stale, tn, tmax, index;
  unsigned int len, hash_val;
  char *metric_type;
  char *p;
                          float f;

  if(! strcmp("METRIC", el) )
     {
        is_string   = 0;
        is_constant = 0;
        is_stale    = 1;
        tn          = 999;
        tmax        = 1;

        for(i = 0; attr[i] ; i+=2)
           {
              if(! strcmp( attr[i], "TYPE" ))
                 {
                    metric_type = (char *) attr[i+1];
                    if(! strcmp( metric_type, "string" ))
                       {
                          is_string = 1;
                       }
                 }
              else if(! strcmp( attr[i], "SLOPE"))
                 {
                    if(! strcmp( attr[i+1], "zero" ))
                       {
                          is_constant = 1;
                       }
                 }
              else if(! strcmp( attr[i], "NAME"))
                 {
                    metric = (char *)attr[i+1];
                 }
              else if(! strcmp( attr[i], "VAL"))
                 {
                    metric_val = (char *)attr[i+1];
                 }
              else if(! strcmp( attr[i], "TN"))
                 {
                    tn = atoi( attr[i+1] );
                 }
              else if(! strcmp( attr[i], "TMAX"))
                 {
                    tmax = atoi( attr[i+1] );
                 }
           }

        if( tn < tmax*4 )
           {
              is_stale = 0;
           }
   
        if(!(is_string || is_constant || is_stale))
           {
              /* Sum up this value (right now only builtin's are summed) */
              len = strlen(metric);
              if( in_metric_list( metric, len ) )
                 {
                    debug_msg("%s is in the metric hash (builtin)", metric);
                    hash_val = metric_hash(metric, len);
                    debug_msg("[%d][%d] will be incremented %s", sum_num->index , hash_val, metric_val);
/*
                    sum_num->sum[hash_val]+= strtold( metric_val, NULL );
                    sum_num->num[hash_val]++;
*/

                    /* Put in order of most frequent to least */
                    if(!strcmp(metric_type, "float"))
                       {
                          f = strtof( metric_val, &p );
                          debug_msg("float = %f metric_val=%s p=%s", f, metric_val, p-1);
/*
                          sum_num->num[hash_val] ++;
*/
                       }
                    else if(!strcmp(metric_type, "uint32"))
                       {

                       }
                    else if(!strcmp(metric_type, "string"))
                       {

                       }
                    else if(!strcmp(metric_type, "double"))
                       {

                       }
                    else if(!strcmp(metric_type, "timestamp"))
                       {

                       }
                    else if(!strcmp(metric_type, "uint16"))
                       {

                       }
                    else
                       {
                          err_msg("Don't know what do to with type %s", metric_type);
                       }
                 }
              else
                 {
                    debug_msg("%s is NOT in the metric hash (custom)", metric);
                 }               

 
              /* Save the data to a round robin database */
              if( push_data_to_rrd( cluster, host, metric, metric_val) )
                 {
                    /* Pass the error on to save_to_rrd */
                    sum_num->rval = 1;
                    return;
                 }
           } 
     }
  else if(! strcmp("HOST", el) )
     {
        for(i = 0; attr[i] ; i+=2)
           {
              if(! strcmp( attr[i], "NAME" ))
                 {
                    if( host ) free ( host );
                    host = strdup ( attr[i+1] );
                 }
           }

     }
  else if(! strcmp("CLUSTER", el) )
     {
        /* Flush the sums */
        memset( sum_num->sum, 0, MAX_HASH_VALUE);

        /* Flush the nums */
        memset( sum_num->num, 0, MAX_HASH_VALUE);

        for(i = 0; attr[i] ; i+=2)
           {
              if(! strcmp( attr[i], "NAME" ))
                 {
                    if( cluster ) free ( cluster );
                    cluster = strdup ( attr[i+1] );
                 }
           }
     }

  return;
}

static void
end (void *data, const char *el)
{
  sum_num_t *sum_num = (sum_num_t *)data;
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
   sum_num_t sum_num;

   /* No errors (yet) */
   sum_num.rval  = 0;
   sum_num.index = index;

   xml_parser = XML_ParserCreate (NULL);
   if (! xml_parser)
      {
         err_msg("save_to_rrd() unable to create XML parser");
         return 1;
      }

   XML_SetElementHandler (xml_parser, start, end);
   XML_SetUserData       (xml_parser, &sum_num);

   rval = XML_Parse( xml_parser, buf, strlen(buf), 1 );
   if(! rval )
      {
         err_msg ("save_to_rrd() XML_ParseBuffer() error at line %d:\n%s\n",
                         XML_GetCurrentLineNumber (xml_parser),
                         XML_ErrorString (XML_GetErrorCode (xml_parser)));
         return 1;
      }

   XML_ParserFree(xml_parser);
   return sum_num.rval;
}
