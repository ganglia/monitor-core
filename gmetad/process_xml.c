/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>

#include "lib/hash.h"

#include "gmetad.h"

extern int write_data_to_rrd( const char *source, const char *host, const char *metric,
   const char *sum, const char *num, unsigned int step, unsigned int time_polled);
extern int zero_out_summary(datum_t *key, datum_t *val, void *arg);
extern char* getfield(char *buf, short int index);

extern struct xml_tag *in_xml_list (const char *, unsigned int);
extern struct type_tag* in_type_list (const char *, unsigned int);

/* Our root host. */
extern Source_t root;
extern gmetad_config_t gmetad_config;

/* The report method functions (in server.c). */
extern int metric_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg);
extern int metric_report_end(Generic_t *self, client_t *client, void *arg);
extern int host_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg);
extern int host_report_end(Generic_t *self, client_t *client, void *arg);
extern int source_report_start(Generic_t *self, datum_t *key, client_t *client, void *arg);
extern int source_report_end(Generic_t *self,  client_t *client, void *arg);

/* Convension is that "object" pointers (struct pointers in C) are capitalized. */

typedef struct
   {
      int rval;
      int old;  /* This is true if the remote source is < 2.5.x */
      char *sourcename;
      char *hostname;
      data_source_list_t *ds;
      int grid_depth;   /* The number of nested grids at this point. Will begin at zero. */
      int host_alive;   /* True if the current host is alive. */
      unsigned int cluster_localtime;  /* Needed for old hosts. */
      Source_t source; /* The current source structure. */
      Host_t host;  /* The current host structure. */
      Metric_t metric;  /* The current metric structure. */
      hash_t *root;     /* The root authority table (contains our data sources). */
   }
xmldata_t;


/* Authority mode is true if we are within the first level GRID. */
static int
authority_mode(xmldata_t *xmldata)
{
   return xmldata->grid_depth == 0;
}


/* Write a metric summary value to the RRD database. */
static int
finish_processing_source(datum_t *key, datum_t *val, void *arg)
{
   xmldata_t *xmldata = (xmldata_t *) arg;
   char *name, *type;
   char sum[256];
   char num[256];
   Metric_t *metric;
   struct type_tag *tt;

   name = (char*) key->data;
   metric = (Metric_t*) val->data;
   type = getfield(metric->strings, metric->type);

   /* Don't save to RRD if the datasource is dead */
   if( xmldata->ds->dead )
      return 1;

   tt = in_type_list(type, strlen(type));
   if (!tt) return 0;

   switch (tt->type)
      {
         case INT:
            sprintf(sum, "%d", metric->val.int32);
            break;
         case UINT:
            sprintf(sum, "%u", metric->val.uint32);
            break;
         case FLOAT:
            sprintf(sum, "%.*f", (int) metric->precision, metric->val.d);
            break;
         default:
            break;
      }
   sprintf(num, "%u", metric->num);

   /* Save the data to a round robin database. Update both sum and num DSs. */
   debug_msg("Writing Summary data for source %s, metric %s",
      xmldata->sourcename, name);

   xmldata->rval = write_data_to_rrd(xmldata->sourcename, NULL, name,
         sum, num, xmldata->ds->step, xmldata->source.localtime);

   return xmldata->rval;
}

   
/* Populates a Metric_t structure from a list of XML metric attribute strings.
 * We need the type string here because we cannot be sure it comes before
 * the metric value in the attribute list.
 */
static void
fillmetric(const char** attr, Metric_t *metric, const char* type)
{
   int i;
   /* INV: always points to the next free byte in metric.strings buffer. */
   int edge = 0;
   struct type_tag *tt;
   struct xml_tag *xt;
   char *metricval, *p;

   /* For old versions of gmond. */
   metric->slope = -1;

   for(i = 0; attr[i] ; i+=2)
      {
         /* Only process the XML tags that gmetad is interested in */
         xt = in_xml_list (attr[i], strlen(attr[i]));
         if (!xt)
            continue;

         switch( xt->tag )
            {
               case SUM_TAG:
               case VAL_TAG:
                  metricval = (char*) attr[i+1];

                  tt = in_type_list(type, strlen(type));
                  if (!tt) return;

                  switch (tt->type)
                     {
                        case INT:
                           metric->val.int32 = (long) strtol(metricval, (char**) NULL, 10);
                           break;
                        case TIMESTAMP:
                        case UINT:
                           metric->val.uint32 = (unsigned long) strtoul(metricval, (char**) NULL, 10);
                           break;
                        case FLOAT:
                           metric->val.d = (double) strtod(metricval, (char**) NULL);
                           p = strrchr(metricval, '.');
                           if (p) metric->precision = (short int) strlen(p+1);
                           break;
                        case STRING:
                           /* We store string values in the 'valstr' field. */
                           break;
                     }
                  metric->valstr = addstring(metric->strings, &edge, metricval);
                  break;
               case TYPE_TAG:
                  metric->type = addstring(metric->strings, &edge, attr[i+1]);
                  break;
               case UNITS_TAG:
                  metric->units = addstring(metric->strings, &edge, attr[i+1]);
                  break;
               case TN_TAG:
                  metric->tn = atoi(attr[i+1]);
                  break;
               case TMAX_TAG:
                  metric->tmax = atoi(attr[i+1]);
                  break;
               case SLOPE_TAG:
                  metric->slope = addstring(metric->strings, &edge, attr[i+1]);
                  break;
               case SOURCE_TAG:
                  metric->source = addstring(metric->strings, &edge, attr[i+1]);
                  break;
               case NUM_TAG:
                  metric->num = atoi(attr[i+1]);
                  break;
               default:
                  break;
            }
      }
      metric->stringslen = edge;
      
      /* We are ok with growing metric values b/c we write to a full-sized buffer in xmldata. */
}


/* Called when a start tag is encountered.
 * sacerdoti: I wish for true OO capabilities. But we need speed.... */
static void
start (void *data, const char *el, const char **attr)
{
   int i;
   xmldata_t *xmldata = (xmldata_t *)data;
   struct xml_tag *xt;
   struct type_tag *tt;
   datum_t *hash_datum = NULL;
   datum_t *rdatum;
   datum_t hashkey, hashval;
   const char *name = NULL;
   const char *metricval = NULL;
   const char *metricnum = NULL;
   const char *type = NULL;
   int edge;

   Metric_t *metric;
   int do_summary = 0;
   Host_t *host;
   uint32_t tn=0;
   uint32_t tmax=0;
   uint32_t reported=0;
   Source_t *source;
   hash_t *summary;
   hash_t *hosts;  /* The current cluster-hosts table. */

   xt = in_xml_list ((char *) el, strlen(el));
   if (!xt)
      return;

   switch( xt->tag )
      {
         case GRID_TAG:

            /* In non-scalable mode, we ignore GRIDs. */
            if (!gmetad_config.scalable_mode) return;

            /* We do not keep info on nested grids. */
            if (authority_mode(xmldata))
               {
                  /* Get name for hash key */
                  for(i = 0; attr[i]; i+=2)
                     {
                        xt = in_xml_list (attr[i], strlen(attr[i]));
                        if (!xt) continue;

                        if (xt->tag == NAME_TAG)
                           {
                              name = attr[i+1];
                              xmldata->sourcename = realloc(xmldata->sourcename, strlen(name)+1);
                              strcpy(xmldata->sourcename, name);
                              hashkey.data = (void*) name;
                              hashkey.size =  strlen(name) + 1;
                           }
                     }

                  source = &(xmldata->source);

                  hash_datum = hash_lookup(&hashkey, xmldata->root);
                  if (!hash_datum)
                     {
                        memset((void*) source, 0, sizeof(*source));

                        source->id = GRID_NODE;
                        source->report_start = source_report_start;
                        source->report_end = source_report_end;

                        source->metric_summary = hash_create(DEFAULT_METRICSIZE);
                        if (!source->metric_summary)
                           {
                              err_msg("Could not create summary hash for cluster %s", name);
                              return;
                           }
                        source->ds = xmldata->ds;

                        /* Initialize the partial sum lock */
                        source->sum_finished = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
                        pthread_mutex_init(source->sum_finished, NULL);
                     }
                  else
                     {
                        /* Copy the stored grid data into our Source buffer in xmldata. */
                        memcpy(source, hash_datum->data, hash_datum->size);
                        datum_free(hash_datum);
                        source->hosts_up = 0;
                        source->hosts_down = 0;

                        hash_foreach(source->metric_summary, zero_out_summary, NULL);
                     }

                  /* Edge has the same invariant as in fillmetric(). */
                  edge = 0;

                  /* Fill in grid attributes. */
                  for(i = 0; attr[i]; i+=2)
                     {
                        xt = in_xml_list(attr[i], strlen(attr[i]));
                        if (!xt)
                           continue;

                        switch( xt->tag )
                           {
                              case AUTHORITY_TAG:
                                 source->authority_ptr = addstring(source->strings, &edge, attr[i+1]);
                                 break;
                              case LOCALTIME_TAG:
                                 source->localtime = strtoul(attr[i+1], (char **)NULL, 10);
                                 break;
                              default:
                                 break;
                           }
                     }
                  source->stringslen = edge;

                  /* Grab the "partial sum" mutex until we are finished summarizing. */
                  pthread_mutex_lock(source->sum_finished);
               }

            /* Must happen after all processing of this tag. */
            xmldata->grid_depth++;
            debug_msg("Found a <GRID>, depth is now %d", xmldata->grid_depth);

            break;


         case CLUSTER_TAG:

            /* Get name for hash key */
            for(i = 0; attr[i]; i+=2)
               {
                  xt = in_xml_list (attr[i], strlen(attr[i]));
                  if (!xt) continue;

                  if (xt->tag == NAME_TAG)
                     name = attr[i+1];
                   else if (xt->tag == LOCALTIME_TAG)
                      xmldata->cluster_localtime = strtoul(attr[i+1], (char **)NULL, 10);
               }

            /* Only keep cluster details if we are the authority on this cluster. */
            if (!authority_mode(xmldata))
               return;

            source = &(xmldata->source);
            
            xmldata->sourcename = realloc(xmldata->sourcename, strlen(name)+1);
            strcpy(xmldata->sourcename, name);
            hashkey.data = (void*) name;
            hashkey.size =  strlen(name) + 1;

            hash_datum = hash_lookup(&hashkey, xmldata->root);
            if (!hash_datum)
               {
                  memset((void*) source, 0, sizeof(*source));
                  
                  /* Set the identity of this host. */
                  source->id = CLUSTER_NODE;
                  source->report_start = source_report_start;
                  source->report_end = source_report_end;

                  source->authority = hash_create(DEFAULT_CLUSTERSIZE);
                  if (!source->authority)
                     {
                        err_msg("Could not create hash table for cluster %s", name);
                        return;
                     }

                  source->metric_summary = hash_create(DEFAULT_METRICSIZE);
                  if (!source->metric_summary)
                     {
                        err_msg("Could not create summary hash for cluster %s", name);
                        return;
                     }
                  source->ds = xmldata->ds;
                  
                  /* Initialize the partial sum lock */
                  source->sum_finished = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
                  pthread_mutex_init(source->sum_finished, NULL);

                  /* Grab the "partial sum" mutex until we are finished summarizing. */
                  pthread_mutex_lock(source->sum_finished);
               }
            else
               {
                  memcpy(source, hash_datum->data, hash_datum->size);
                  datum_free(hash_datum);

                  source->hosts_up = 0;
                  source->hosts_down = 0;

                  /* We need this lock before zeroing metric sums. */
                  pthread_mutex_lock(source->sum_finished);
                  hash_foreach(source->metric_summary, zero_out_summary, NULL);
               }

            /* Edge has the same invariant as in fillmetric(). */
            edge = 0;

            source->owner = -1;
            source->latlong = -1;
            source->url = -1;
            source->localtime = xmldata->cluster_localtime;
            /* Fill in cluster attributes. */
            for(i = 0; attr[i]; i+=2)
               {
                  xt = in_xml_list (attr[i], strlen(attr[i]));
                  if (!xt)
                     continue;

                  switch( xt->tag )
                     {
                        case OWNER_TAG:
                           source->owner = addstring(source->strings, &edge, attr[i+1]);
                           break;
                        case LATLONG_TAG:
                           source->latlong = addstring(source->strings, &edge, attr[i+1]);
                           break;
                        case URL_TAG:
                           source->url = addstring(source->strings, &edge, attr[i+1]);
                           break;
                        default:
                           break;
                     }
               }
            source->stringslen = edge;

            break;


         case HOST_TAG:

            /* Check if the host is up. */
            for (i = 0; attr[i]; i+=2)
               {
                  xt = in_xml_list (attr[i], strlen(attr[i]));
                  if (!xt) continue;

                  if (xt->tag == REPORTED_TAG)
                     reported = strtoul(attr[i+1], (char **)NULL, 10);
                  else if (xt->tag == TN_TAG)
                     tn = atoi(attr[i+1]);
                  else if (xt->tag == TMAX_TAG)
                     tmax = atoi(attr[i+1]);
                  else if (xt->tag == NAME_TAG)
                     name = attr[i+1];
               }

            /* Is this host alive? For pre-2.5.0, we use the 60-second 
             * method, otherwise we use the host's TN and TMAX attrs.
             */
            xmldata->host_alive = (xmldata->old || !tmax) ?
               abs(xmldata->cluster_localtime - reported) < 60 :
               tn < tmax * 4;

            if (xmldata->host_alive)
               xmldata->source.hosts_up++;
            else
               xmldata->source.hosts_down++;

            /* Only keep host details if we are the authority on this cluster. */
            if (!authority_mode(xmldata))
               return;

            host = &(xmldata->host);

            /* Use node Name for hash key (Query processing
             * requires a name key).
             */
            xmldata->hostname = realloc(xmldata->hostname, strlen(name)+1);
            strcpy(xmldata->hostname, name);
            hashkey.data = (void*) name;
            hashkey.size =  strlen(name) + 1;

            hosts = xmldata->source.authority;
            if(hosts)
              {
                hash_datum = hash_lookup (&hashkey, hosts);
              }
            else
              {
                hash_datum = NULL;
              }
            if (!hash_datum)
               {
                  memset((void*) host, 0, sizeof(*source));

                  host->id = HOST_NODE;
                  host->report_start = host_report_start;
                  host->report_end = host_report_end;

                  /* Only create one hash table for the host's metrics. Not user/builtin like gmond. */
                  host->metrics = hash_create(DEFAULT_METRICSIZE);
                  if (!host->metrics)
                     {
                        err_msg("Could not create metric hash for host %s", name);
                        return;
                     }
               }
            else
               {
                  /* Copy the stored host data into our Host buffer in xmldata. */
                  memcpy(host, hash_datum->data, hash_datum->size);
                  datum_free(hash_datum);
               }

            /* Edge has the same invariant as in fillmetric(). */
            edge = 0;

            host->location = -1;
            host->reported = reported;
            host->tn = tn;
            host->tmax = tmax;

            /* We will store this host in the cluster's authority table. */
            for(i = 0; attr[i]; i+=2)
               {
                  xt = in_xml_list (attr[i], strlen(attr[i]));
                  if (!xt) continue;

                  switch( xt->tag )
                     {
                        case IP_TAG:
                           host->ip = addstring(host->strings, &edge, attr[i+1]);
                           break;
                        case DMAX_TAG:
                           host->dmax = strtoul(attr[i+1], (char **)NULL, 10);
                           break;
                        case LOCATION_TAG:
                           host->location = addstring(host->strings, &edge, attr[i+1]);
                           break;
                        case STARTED_TAG:
                           host->started = strtoul(attr[i+1], (char **)NULL, 10);
                           break;
                        default:
                           break;
                     }
               }
            host->stringslen = edge;

            /* Trim structure to the correct length. */
            hashval.size = sizeof(*host) - FRAMESIZE + host->stringslen;
            hashval.data = host;

            /* We dont care if this is an insert or an update. */
            if(hosts)
              {
                rdatum = hash_insert(&hashkey, &hashval, hosts);
                if (!rdatum)
                  {
                     err_msg("Could not insert host %s", name);
                  }
              }

            break;


         case HOSTS_TAG:

            /* In non-scalable mode, we do not process summary data. */
            if (!gmetad_config.scalable_mode) return;

            /* Add up/down hosts to this grid summary */
            for(i = 0; attr[i]; i+=2)
               {
                  xt = in_xml_list (attr[i], strlen(attr[i]));
                  if (!xt)
                     continue;

                  switch( xt->tag )
                     {
                        case UP_TAG:
                           xmldata->source.hosts_up += strtoul(attr[i+1], (char **)NULL, 10);
                           break;
                        case DOWN_TAG:
                           xmldata->source.hosts_down += strtoul(attr[i+1], (char **)NULL, 10);
                           break;
                        default:
                           break;
                     }
               }
               
               break;
               

         case METRIC_TAG:

            if (!xmldata->host_alive) return;

            /* Get name for hash key, and val/type for summaries. */
            for(i = 0; attr[i]; i+=2)
               {
                  xt = in_xml_list(attr[i], strlen(attr[i]));
                  if (!xt) continue;

                  switch (xt->tag)
                     {
                        case NAME_TAG:
                           name = attr[i+1];
                           hashkey.data = (void*) name;
                           hashkey.size =  strlen(name) + 1;
                           break;
                        case VAL_TAG:
                           metricval = attr[i+1];
                           break;
                        case TYPE_TAG:
                           type = attr[i+1];
                           break;
                        default:
                           break;
                     }
               }

            metric = &(xmldata->metric);
            memset((void*) metric, 0, sizeof(*metric));

            /* Summarize all numeric metrics */
            do_summary = 0;
            tt = in_type_list(type, strlen(type));
            if (!tt) return;

            if (tt->type==INT || tt->type==UINT || tt->type==FLOAT)
               do_summary = 1;

            /* Only keep metric details if we are the authority on this cluster. */
            if (authority_mode(xmldata))
               {
                  /* Save the data to a round robin database if the data source is alive */
                  if (do_summary && !xmldata->ds->dead && !xmldata->rval)
                     {
                           debug_msg("Updating host %s, metric %s", xmldata->hostname, name);
                           xmldata->rval = write_data_to_rrd(xmldata->sourcename,
                                 xmldata->hostname, name, metricval, NULL,
                                 xmldata->ds->step, xmldata->source.localtime);
                     }
                  metric->id = METRIC_NODE;
                  metric->report_start = metric_report_start;
                  metric->report_end = metric_report_end;

                  fillmetric(attr, metric, type);

                  /* Trim metric structure to the correct length. */
                  hashval.size = sizeof(*metric) - FRAMESIZE + metric->stringslen;
                  hashval.data = (void*) metric;

                  /* Update full metric in cluster host table. */
                  rdatum = hash_insert(&hashkey, &hashval, xmldata->host.metrics);
                  if (!rdatum)
                     {
                        err_msg("Could not insert %s metric", name);
                     }
               }

            /* Always update summary for numeric metrics. */
            if (do_summary)
               {
                  summary = xmldata->source.metric_summary;
                  hash_datum = hash_lookup(&hashkey, summary);
                  if (!hash_datum)
                     {
                        if (!authority_mode(xmldata))
                           {
                              metric = &(xmldata->metric);
                              memset((void*) metric, 0, sizeof(*metric));
                              fillmetric(attr, metric, type);
                           }
                        /* else we have already filled in the metric above. */
                     }
                  else
                     {
                        memcpy(&xmldata->metric, hash_datum->data, hash_datum->size);
                        datum_free(hash_datum);
                        metric = &(xmldata->metric);

                        switch (tt->type)
                           {
                              case INT:
                                 metric->val.int32 += (long) strtol(metricval, (char**) NULL, 10);
                                 break;
                              case UINT:
                                 metric->val.uint32 += (unsigned long) strtoul(metricval, (char**) NULL, 10);
                                 break;
                              case FLOAT:
                                 metric->val.d += (double) strtod(metricval, (char**) NULL);
                                 break;
                              default:
                                 break;
                           }
                     }

                  metric->num++;

                  /* Trim metric structure to the correct length. Tricky. */
                  hashval.size = sizeof(*metric) - FRAMESIZE + metric->stringslen;
                  hashval.data = (void*) metric;

                  /* Update metric in summary table. */
                  rdatum = hash_insert(&hashkey, &hashval, summary);
                  if (!rdatum) err_msg("Could not insert %s metric", name);
               }
            break;


         case METRICS_TAG:

            /* In non-scalable mode, we do not process summary data. */
            if (!gmetad_config.scalable_mode) return;
            
            /* Get name for hash key, and val/type for summaries. */
            for(i = 0; attr[i]; i+=2)
               {
                  xt = in_xml_list(attr[i], strlen(attr[i]));
                  if (!xt) continue;

                  switch (xt->tag)
                     {
                        case NAME_TAG:
                           name = attr[i+1];
                           hashkey.data = (void*) name;
                           hashkey.size =  strlen(name) + 1;
                           break;
                        case TYPE_TAG:
                           type = attr[i+1];
                           break;
                        case SUM_TAG:
                           metricval = attr[i+1];
                           break;
                        case NUM_TAG:
                           metricnum = attr[i+1];
                        default:
                           break;
                     }
               }

            summary = xmldata->source.metric_summary;
            hash_datum = hash_lookup(&hashkey, summary);
            if (!hash_datum)
               {
                  metric = &(xmldata->metric);
                  memset((void*) metric, 0, sizeof(*metric));
                  fillmetric(attr, metric, type);
               }
            else
               {
                  memcpy(&xmldata->metric, hash_datum->data, hash_datum->size);
                  datum_free(hash_datum);
                  metric = &(xmldata->metric);

                  tt = in_type_list(type, strlen(type));
                  if (!tt) return;
                  switch (tt->type)
                        {
                           case INT:
                              metric->val.int32 += (long) strtol(metricval, (char**) NULL, 10);
                              break;
                           case UINT:
                              metric->val.uint32 += (unsigned long) strtoul(metricval, (char**) NULL, 10);
                              break;
                           case FLOAT:
                              metric->val.d += (double) strtod(metricval, (char**) NULL);
                              break;
                           default:
                              break;
                        }
                     metric->num += atoi(metricnum);
                  }

            /* Update metric in summary table. */
            hashval.size = sizeof(*metric) - FRAMESIZE + metric->stringslen;
            hashval.data = (void*) metric;

            summary = xmldata->source.metric_summary;
            rdatum = hash_insert(&hashkey, &hashval, summary);
            if (!rdatum) err_msg("Could not insert %s metric", name);

            break;


         case GANGLIA_XML_TAG:

            for(i = 0; attr[i] ; i+=2)
               {
                  /* Only process the XML tags that gmetad is interested in */
                  if( !( xt = in_xml_list ( (char *)attr[i], strlen(attr[i]))) )
                     continue;

                  if (xt->tag == VERSION_TAG)
                     {
                           /* Process the version tag later */
                           if(! strstr( attr[i+1], "2.5." ) )
                              {
                                 debug_msg("[%s] is an OLD version", xmldata->ds->name);
                                 xmldata->old = 1;
                              }
                     }
               }
               break;

            default:
               break;

      }  /* end switch */

   return;
}


static void
end (void *data, const char *el)
{
   xmldata_t *xmldata = (xmldata_t *) data;
   datum_t hashkey, hashval;
   datum_t *rdatum;
   hash_t *summary;
   Source_t *source;
   struct xml_tag *xt;

   if(! (xt = in_xml_list ( (char *)el, strlen( el ))) )
      return;

   switch ( xt->tag )
      {
         /* </GRID> */
         case GRID_TAG:

            if (gmetad_config.scalable_mode)
               {
                  xmldata->grid_depth--;
                  debug_msg("Found a </GRID>, depth is now %d", xmldata->grid_depth);
               }
            /* No break. */

         /* </CLUSTER> */
         case CLUSTER_TAG:

            /* Only keep info on sources we are an authority on. */
            if (authority_mode(xmldata))
               {
                  source = &xmldata->source;
                  summary = xmldata->source.metric_summary;

                  /* Release the partial sum mutex */
                  pthread_mutex_unlock(source->sum_finished);
                  /*err_msg("%s releasing lock", xmldata->sourcename);*/

                  hashkey.data = (void*) xmldata->sourcename;
                  hashkey.size = strlen(xmldata->sourcename) + 1;

                  hashval.data = source;
                  /* Trim structure to the correct length. */
                  hashval.size = sizeof(*source) - FRAMESIZE + source->stringslen;

                  /* We insert here to get an accurate hosts up/down value. */
                  rdatum = hash_insert( &hashkey, &hashval, xmldata->root);
                  if (!rdatum)
                     {
                        err_msg("Could not insert source %s", xmldata->sourcename);
                     }
                  /* Write the metric summaries to the RRD. */
                  hash_foreach(summary, finish_processing_source, data);
               }

            break;

         default:
               break;
      }
   return;
}


/* Starts the Expat parser on the XML tree from this data source. */
int
process_xml(data_source_list_t *d, char *buf)
{
   int rval;
   XML_Parser xml_parser;
   xmldata_t xmldata;
   char now[32];

   memset( &xmldata, 0, sizeof( xmldata ));

   /* Set the return value to zero by default */
   xmldata.rval = 0;

   /* Set the pointer to the data source record */
   xmldata.ds = d;
   
   /* Set the hash table for the root data source. */
   xmldata.root = root.authority;

   xml_parser = XML_ParserCreate (NULL);
   if (! xml_parser)
      {
         err_msg("Process XML: unable to create XML parser");
         return 1;
      }

   XML_SetElementHandler (xml_parser, start, end);
   XML_SetUserData (xml_parser, &xmldata);

   if( gmetad_config.force_names )
     {
       snprintf(now, 32, "%d", d->last_heard_from);
       XML_Parse(xml_parser, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?><GANGLIA_XML VERSION=\"0.0.0\" SOURCE=\"gmond\"><CLUSTER NAME=\"", 119, 0);
       XML_Parse(xml_parser, d->name, strlen(d->name), 0);  
       XML_Parse(xml_parser, "\" LOCALTIME=\"", 13, 0);
       XML_Parse(xml_parser, now, strlen(now), 0);
       XML_Parse(xml_parser, "\" OWNER=\"unspecified\" LATLONG=\"unspecified\" URL=\"unspecified\">", 62, 0 );
     }

   if(buf)
     {
       rval= XML_Parse( xml_parser, buf, strlen(buf), gmetad_config.force_names? 0: 1 );
       if(! rval )
          {
             err_msg ("Process XML (%s): XML_ParseBuffer() error:\n%s\n",
                             d->name,
                             XML_ErrorString (XML_GetErrorCode (xml_parser)));
             xmldata.rval = 1;
          }
     }

   if( xmldata.rval == 0 && gmetad_config.force_names )
     {
       XML_Parse(xml_parser, "</CLUSTER></GANGLIA_XML>",24, 1 );
     }

   /* Release lock again for good measure (required under certain errors). */
   if (xmldata.source.sum_finished)
      {
         rval = pthread_mutex_unlock(xmldata.source.sum_finished);
         if (rval!=0)
            err_msg("Could not release summary lock for %s", d->name);
      }

   /* Free memory that might have been allocated in xmldata */
   if (xmldata.sourcename)
      free(xmldata.sourcename);

   if (xmldata.hostname)
      free(xmldata.hostname);

   XML_ParserFree(xml_parser);
   return xmldata.rval;
}
