/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ganglia/hash.h>
#include <ganglia/xmlparse.h>
#include <gmetad.h>

extern int write_data_to_rrd( char *source, char *host, char *metric, char *sum, char *num, unsigned int step);

extern struct xml_tag *in_xml_list (char *, unsigned int);
extern struct ganglia_metric *in_metric_list (char *, unsigned int);
extern struct ganglia_metric metrics[];
extern char *gridname;     /* Our grid name */

typedef struct
   {
      int rval;
      int old;  /* This is true if the remote source is < 2.5.x */
      long double  overall_sum[MAX_METRIC_HASH_VALUE];
      unsigned int overall_num[MAX_METRIC_HASH_VALUE];
      long double          sum[MAX_METRIC_HASH_VALUE];
      unsigned int         num[MAX_METRIC_HASH_VALUE];
      char *source;   
      char *host;
      char *metric;
      char *metric_val;
      data_source_list_t *ds;
      unsigned int cluster_localtime;
      unsigned int host_reported; 
      int grid_depth;   /* The number of nested grids at this point. Will begin at zero. */
   }
xml_data_t;


static void
start_processing_source (xml_data_t *xml_data, const char **attr)
{
   register int i;
   struct xml_tag *xt;

   if (xml_data->grid_depth==0) {
      /* Flush the sums and nums. */
      memset( xml_data->sum, 0, MAX_METRIC_HASH_VALUE * sizeof( long double));
      memset( xml_data->num, 0, MAX_METRIC_HASH_VALUE * sizeof( unsigned int));
   }

   for(i = 0; attr[i] ; i+=2) {
      /* Only process the XML tags that gmetad is interested in */
      if ( !( xt = in_xml_list ( (char *)attr[i], strlen(attr[i]))) )
         continue;

      switch( xt->tag )  {

         case LOCALTIME_TAG:
            xml_data->cluster_localtime = strtoul(attr[i+1], (char **)NULL, 10);
            break;
         case NAME_TAG:
            /* This source can either be a grid name or a cluster name, but should be the most outer
               name if we are summarizing accross a grid. */
            if (xml_data->grid_depth != 0) break;

            xml_data->source = realloc ( xml_data->source, strlen(attr[i+1])+1 );
            strcpy( xml_data->source, attr[i+1] );
            break;
      }
   }
}

static void
finish_processing_source(xml_data_t *xml_data)
{
   register int i;
   int len;
   struct xml_tag *xt;
   struct ganglia_metric *gm;
   char sum[512];
   char num[512];

   /* We only write our summary data to the RRDs when the grid_depth is zero
      (we summarize over our entire child grids). */
   if (xml_data->grid_depth !=0) return;

   for ( i = 0; i < MAX_METRIC_HASH_VALUE; i++ )
      {
         len = strlen(metrics[i].name);
         if( len )
            {
                  gm  =  (struct ganglia_metric *)in_metric_list ((char *)metrics[i].name, len);

                  /* Skip it if we have no hosts reporting the data */
                  if (! xml_data->num[i] )
                     continue;

                  /* Don't save to RRD if the datasource is dead */
                  if( xml_data->ds->dead )
                     continue;

                  sprintf( sum, "%Lf", xml_data->sum[i] );
                  sprintf( num, "%d",  xml_data->num[i] );

                  /* Save the data to a round robin database */
                  debug_msg("Writing Summary data for source %s", xml_data->source);
                  xml_data->rval = write_data_to_rrd(xml_data->source, NULL, (char*) metrics[i].name,
                                                                                 sum, num, xml_data->ds->step);
                  if (xml_data->rval)
                     {
                        /* Pass the error on to save_to_rrd */
                        return;
                     }

                  /* Increment the overall sum and num */
                  xml_data->overall_sum[i] +=  xml_data->sum[i];
                  xml_data->overall_num[i] +=  xml_data->num[i];
            }
      }
}

static void
start (void *data, const char *el, const char **attr)
{
  register int i;
  xml_data_t *xml_data = (xml_data_t *)data;
  int tn, tmax, index, is_volatile, is_numeric, len, blessed;
  struct ganglia_metric *gm;
  struct xml_tag *xt;

  /* We got an error before */
  if( xml_data->rval )
     return;

  if(! (xt = in_xml_list ( (char *)el, strlen( el ))) )
     return;

  switch( xt->tag )
     {

        case METRIC_TAG:

           tn          = 0;
           tmax        = 0;
           is_volatile = 0;
           is_numeric  = 0;
           blessed     = 0;
           gm          = NULL;

           for(i = 0; attr[i] ; i+=2)
              {
                 /* Only process the XML tags that gmetad is interested in */
                 if(!( xt = in_xml_list ( (char *)attr[i], strlen(attr[i]))) )
                    continue;

                 switch( xt->tag )
                    {
                       case NAME_TAG:
                          xml_data->metric = (char *)attr[i+1];
                          if( gm = in_metric_list( (char *)(xml_data->metric), strlen( xml_data->metric) ) )
                             blessed = 1;
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
                             {
                                is_volatile = 1; 
                             }
                          break;

                       case TYPE_TAG:
                          /* TYPE != string */
                          if( strcmp( attr[i+1], "string" ))
                             {
                                is_numeric = 1;
                             }
                          break;
                    }
              }

           /* For pre-2.5.0, check if the host is up this way */
           if ( xml_data->old && abs(xml_data->cluster_localtime - xml_data->host_reported) > 60 )
              return;
 
           /* Only process fresh data, volatile, numeric data (or blessed) */
           if (  tn > tmax*4 )
              return;

           if( !( (is_volatile && is_numeric) || blessed))
              return;

           if( gm )
              {
                 unsigned int hash_val;

                 /* We are going to sum all builtin metrics for summary RRDs */
                 /*debug_msg("%s is in the metric hash (builtin or blessed)", xml_data->metric);*/

                 hash_val = metric_hash( xml_data->metric, strlen(xml_data->metric) );

                 switch ( gm->type )
                    {
                       case FLOAT:
                          xml_data->sum[hash_val] +=  (long double)(strtod( (const char *)(xml_data->metric_val), (char **)NULL));
                          xml_data->num[hash_val]++;
                          /* debug_msg("%d sum = %Lf num = %d", hash_val, xml_data->sum[hash_val], xml_data->num[hash_val] ); */
                          break;
                       case UINT32:
                          xml_data->sum[hash_val] += (long double)(strtoul(xml_data->metric_val, (char **)NULL, 10));
                          xml_data->num[hash_val]++;
                         /* debug_msg("%d sum = %Lf num = %d", hash_val, xml_data->sum[hash_val], xml_data->num[hash_val]);*/
                          break;
                       case DOUBLE:
                          xml_data->sum[hash_val] += (long double)(strtod( (const char *)(xml_data->metric_val), (char **)NULL));
                          xml_data->num[hash_val]++;
                          /*debug_msg("%d sum = %Lf num = %d", hash_val, xml_data->sum[hash_val], xml_data->num[hash_val]);*/
                          break;
                    }
              }

           /* Save the data to a round robin database if the data source is alive */
           if( xml_data->ds->dead )
              return;

           /* Only update host RRDs if no other gmetad is doing it. */
           if (xml_data->grid_depth==0) 
           {
                xml_data->rval = write_data_to_rrd( xml_data->source, xml_data->host, xml_data->metric,
                                                                                 xml_data->metric_val, NULL, xml_data->ds->step);
                if (xml_data->rval)  {
                     return;
                }
           }
           break;

        case HOST_TAG:

           for(i = 0; attr[i] ; i+=2)
              {
                 if(!( xt = in_xml_list ( (char *)attr[i], strlen(attr[i]))) )
                    continue;

                 switch( xt->tag )
                    {
                       case REPORTED_TAG:
                          xml_data->host_reported = strtoul(attr[i+1], (char **)NULL, 10); 
                          break;
                       case NAME_TAG:
                          xml_data->host = realloc( xml_data->host, strlen(attr[i+1])+1 );
                          strcpy( xml_data->host, attr[i+1] ); 
                          break;
                    }
              }
           break;

        case GRID_TAG:

           start_processing_source(xml_data, attr);

           /* Must happen after call to process_source. */
           xml_data->grid_depth++;
           debug_msg("Found a <GRID>, depth is now %d", xml_data->grid_depth);

           break;

        case CLUSTER_TAG:

           start_processing_source(xml_data, attr);

           break;

        case GANGLIA_XML_TAG:

           for(i = 0; attr[i] ; i+=2)
              {
                 /* Only process the XML tags that gmetad is interested in */
                 if(!( xt = in_xml_list ( (char *)attr[i], strlen(attr[i]))) )
                    continue;

                 switch( xt->tag )
                    {
                       case VERSION_TAG:
                          /* Process the version tag later */
                          if(! strstr( attr[i+1], "2.5." ) )
                             {
                                debug_msg("[%s] is an OLD version", xml_data->ds->name);
                                xml_data->old = 1;
                             }   
                          break;
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
  struct xml_tag *xt;

  if(! (xt = in_xml_list ( (char *)el, strlen( el ))) )
     return;

  switch ( xt->tag )
     {
        /* </GANGLIA_XML> */
        case GANGLIA_XML_TAG:

           /* Save to source */
           memcpy( xml_data->ds->num, xml_data->overall_num, MAX_METRIC_HASH_VALUE * sizeof(unsigned int) );
           memcpy( xml_data->ds->sum, xml_data->overall_sum, MAX_METRIC_HASH_VALUE * sizeof(long double) );

           break;

        /* </GRID> */
        case GRID_TAG:

           xml_data->grid_depth--;
           debug_msg("Found a </GRID>, depth is now %d", xml_data->grid_depth);
           
           finish_processing_source(xml_data);

           break;

         /* </CLUSTER> */
        case CLUSTER_TAG:

           finish_processing_source(xml_data);

           break;

     }
  return;
}

int
process_xml(data_source_list_t *d, char *buf)
{
   int rval;
   XML_Parser xml_parser;
   xml_data_t xml_data;

   memset( &xml_data, 0, sizeof( xml_data ));

   /* Set the pointer to the data source record */
   xml_data.ds = d;

   xml_parser = XML_ParserCreate (NULL);
   if (! xml_parser)
      {
         err_msg("Process XML: unable to create XML parser");
         return 1;
      }

   XML_SetElementHandler (xml_parser, start, end);
   XML_SetUserData       (xml_parser, &xml_data);

   rval = XML_Parse( xml_parser, buf, strlen(buf), 1 );

   if(! rval )
      {
         err_msg ("Process XML: XML_ParseBuffer() error at line %d:\n%s\n",
                         XML_GetCurrentLineNumber (xml_parser),
                         XML_ErrorString (XML_GetErrorCode (xml_parser)));
         xml_data.rval = 1;
      }

   /* Free memory that might have been allocated in xml_data */
   if( xml_data.source )
      {
         free(xml_data.source);
         xml_data.source = NULL;
      }
   if( xml_data.host )
      {
         free(xml_data.host);
         xml_data.host = NULL;
      }

   XML_ParserFree(xml_parser);
   return xml_data.rval;
}
