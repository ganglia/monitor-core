#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <expat.h>
#include <ganglia.h>
#include "gmetad.h"
#include "rrd_helpers.h"
#include "export_helpers.h"

extern int zero_out_summary(datum_t *key, datum_t *val, void *arg);
extern char* getfield(char *buf, short int index);

extern struct xml_tag *in_xml_list (const char *, unsigned int);
extern struct type_tag* in_type_list (const char *, unsigned int);

/* Our root host. */
extern Source_t root;
extern gmetad_config_t gmetad_config;

#ifdef WITH_RIEMANN
#include "riemann.pb-c.h"
__thread Msg *riemann_msg = NULL;
__thread int riemann_num_events;
#endif /* WITH_RIEMANN */

/* The report method functions (in server.c). */
extern int metric_report_start(Generic_t *self, datum_t *key, 
                client_t *client, void *arg);
extern int metric_report_end(Generic_t *self, client_t *client, void *arg);
extern int host_report_start(Generic_t *self, datum_t *key, 
                client_t *client, void *arg);
extern int host_report_end(Generic_t *self, client_t *client, void *arg);
extern int source_report_start(Generic_t *self, datum_t *key, 
                client_t *client, void *arg);
extern int source_report_end(Generic_t *self,  client_t *client, void *arg);

/* In gmeta.c */
extern int addstring(char *strings, int *edge, const char *s);

/* Convension is that "object" pointers (struct pointers in C) are capitalized.
 */

typedef struct
   {
      int rval;
      int old;  /* This is true if the remote source is < 2.5.x */
      char *sourcename;
      char *hostname;
      data_source_list_t *ds;
      int grid_depth;   /* The number of nested grids at this point. Will begin
                           at zero. */
      int host_alive;   /* True if the current host is alive. */
      Source_t source; /* The current source structure. */
      Host_t host;  /* The current host structure. */
      Metric_t metric;  /* The current metric structure. */
      hash_t *root;     /* The root authority table (contains our data
                           sources). */ 
      struct timeval now;
}
xmldata_t;


/* Authority mode is true if we are within the first level GRID. */
static int
authority_mode(xmldata_t *xmldata)
{
   return xmldata->grid_depth == 0;
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
                        case TIMESTAMP:
                        case UINT:
                        case FLOAT:
                           metric->val.d = (double)
                                   strtod(metricval, (char**) NULL);
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
               case DMAX_TAG:
                  metric->dmax = atoi(attr[i+1]);
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
      
      /* We are ok with growing metric values b/c we write to a full-sized
       * buffer in xmldata. */
}


static int
startElement_GRID(void *data, const char *el, const char **attr)
{
   xmldata_t *xmldata = (xmldata_t *)data;
   struct xml_tag *xt;
   datum_t *hash_datum = NULL;
   datum_t hashkey;
   const char *name = NULL;
   int edge;
   int i;
   Source_t *source;

   /* In non-scalable mode, we ignore GRIDs. */
   if (!gmetad_config.scalable_mode) 
      return 0;

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
                     xmldata->sourcename = 
                             realloc(xmldata->sourcename, strlen(name)+1);
                     strcpy(xmldata->sourcename, name);
                     hashkey.data = (void*) name;
                     hashkey.size =  strlen(name) + 1;
                  }
            }

         source = &(xmldata->source);

         /* Query the hash table for this cluster */
         hash_datum = hash_lookup(&hashkey, xmldata->root);
         if (!hash_datum)  
            {  /* New Cluster */
               memset((void*) source, 0, sizeof(*source));

               source->id = GRID_NODE;
               source->report_start = source_report_start;
               source->report_end = source_report_end;

               source->metric_summary = hash_create(DEFAULT_METRICSIZE);
               if (!source->metric_summary)
                  {
                     err_msg("Could not create summary hash for cluster %s", 
                                     name);
                     return 1;
                  }
               source->ds = xmldata->ds;

               /* Initialize the partial sum lock */
               source->sum_finished = (pthread_mutex_t *) 
                       malloc(sizeof(pthread_mutex_t));
               pthread_mutex_init(source->sum_finished, NULL);

               /* Grab the "partial sum" mutex until we are finished
                * summarizing. */
               pthread_mutex_lock(source->sum_finished);
            }
         else
            {  /* Found Cluster. Put into our Source buffer in xmldata. */
               memcpy(source, hash_datum->data, hash_datum->size);
               datum_free(hash_datum);

               /* Grab the "partial sum" mutex until we are finished
                * summarizing. Needs to be done asap.*/
               pthread_mutex_lock(source->sum_finished);

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
                        source->authority_ptr = addstring(source->strings, 
                                        &edge, attr[i+1]);
                        break;
                     case LOCALTIME_TAG:
                        source->localtime = strtoul(attr[i+1], 
                                        (char **) NULL, 10);
                        break;
                     default:
                        break;
                  }
            }
         source->stringslen = edge;
      }

   /* Must happen after all processing of this tag. */
   xmldata->grid_depth++;
   debug_msg("Found a <GRID>, depth is now %d", xmldata->grid_depth);

   return 0;
}


static int
startElement_CLUSTER(void *data, const char *el, const char **attr)
{
   xmldata_t *xmldata = (xmldata_t *)data;
   struct xml_tag *xt;
   datum_t *hash_datum = NULL;
   datum_t hashkey;
   const char *name = NULL;
   int edge;
   int i;
   Source_t *source;

   /* Get name for hash key */
   for(i = 0; attr[i]; i+=2)
      {
         xt = in_xml_list (attr[i], strlen(attr[i]));
         if (!xt) continue;

         if (xt->tag == NAME_TAG)
            name = attr[i+1];
      }

   /* Only keep cluster details if we are the authority on this cluster. */
   if (!authority_mode(xmldata))
      return 0;

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
               return 1;
            }
         if(gmetad_config.case_sensitive_hostnames == 0)
            hash_set_flags(source->authority, HASH_FLAG_IGNORE_CASE);

         source->metric_summary = hash_create(DEFAULT_METRICSIZE);
         if (!source->metric_summary)
            {
               err_msg("Could not create summary hash for cluster %s", name);
               return 1;
            }
         source->ds = xmldata->ds;
         
         /* Initialize the partial sum lock */
         source->sum_finished = (pthread_mutex_t *) 
                 malloc(sizeof(pthread_mutex_t));
         pthread_mutex_init(source->sum_finished, NULL);

         /* Grab the "partial sum" mutex until we are finished summarizing. */
         pthread_mutex_lock(source->sum_finished);
      }
   else
      {
         memcpy(source, hash_datum->data, hash_datum->size);
         datum_free(hash_datum);

         /* We need this lock before zeroing metric sums. */
         pthread_mutex_lock(source->sum_finished);

         source->hosts_up = 0;
         source->hosts_down = 0;

         hash_foreach(source->metric_summary, zero_out_summary, NULL);
      }

   /* Edge has the same invariant as in fillmetric(). */
   edge = 0;

   source->owner = -1;
   source->latlong = -1;
   source->url = -1;
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
                  source->latlong = addstring(source->strings, 
                                  &edge, attr[i+1]);
                  break;
               case URL_TAG:
                  source->url = addstring(source->strings, &edge, attr[i+1]);
                  break;
               case LOCALTIME_TAG:
                  source->localtime = strtoul(attr[i+1],
                                (char **) NULL, 10);
                  break;
               default:
                  break;
            }
      }
   source->stringslen = edge;
   return 0;
}


static int
startElement_HOST(void *data, const char *el, const char **attr)
{
   xmldata_t *xmldata = (xmldata_t *)data;
   datum_t *hash_datum = NULL;
   datum_t *rdatum;
   datum_t hashkey, hashval;
   struct xml_tag *xt;
   uint32_t tn=0;
   uint32_t tmax=0;
   uint32_t reported=0;
   const char *name = NULL;
   int edge;
   int i;
   Host_t *host;
   hash_t *hosts;  

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
      abs(xmldata->source.localtime - reported) < 60 :
      tn < tmax * 4;

   if (xmldata->host_alive)
      xmldata->source.hosts_up++;
   else
      xmldata->source.hosts_down++;

   /* Only keep host details if we are the authority on this cluster. */
   if (!authority_mode(xmldata))
      return 0;

   host = &(xmldata->host);

   /* Use node Name for hash key (Query processing
    * requires a name key).
    */
   xmldata->hostname = realloc(xmldata->hostname, strlen(name)+1);
   strcpy(xmldata->hostname, name);

   /* Convert name to lower case - host names can't be
    * case sensitive
    */
   /*for(i = 0; name[i] != 0; i++)
       xmldata->hostname[i] = tolower(name[i]);
   xmldata->hostname[i] = 0; */

   hashkey.data = (void*) name;
   hashkey.size =  strlen(name) + 1;

   hosts = xmldata->source.authority;
   hash_datum = hash_lookup (&hashkey, hosts);
   if (!hash_datum)
      {
         memset((void*) host, 0, sizeof(*host));

         host->id = HOST_NODE;
         host->report_start = host_report_start;
         host->report_end = host_report_end;

         /* Only create one hash table for the host's metrics. Not user/builtin
          * like gmond. */
         host->metrics = hash_create(DEFAULT_METRICSIZE);
         if (!host->metrics)
            {
               err_msg("Could not create metric hash for host %s", name);
               return 1;
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
   host->tags = -1;
   host->reported = reported;
   host->tn = tn;
   host->tmax = tmax;
   /* sacerdoti: Host TN tracks what gmond sees. TN=0 when gmond received last
    * heartbeat from node. Works because clocks move at same speed, and TN is
    * a relative timespan. */
   host->t0 = xmldata->now;
   host->t0.tv_sec -= host->tn;

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
	       case TAGS_TAG:
		  host->tags = addstring(host->strings, &edge, attr[i+1]);
		  break;
               case STARTED_TAG:
                  host->started = strtoul(attr[i+1], (char **)NULL, 10);
                  break;
               default:
                  break;
            }
      }
   host->stringslen = edge;

#ifdef WITH_RIEMANN

   /* Forward heartbeat metric to Riemann */
   if (gmetad_config.riemann_server) {

      if (!strcmp(gmetad_config.riemann_protocol, "tcp")) {
         riemann_msg = malloc (sizeof (Msg));
         msg__init (riemann_msg);
      }

      char value[12];
      sprintf(value, "%d", reported);

      Event *event = create_riemann_event (gmetad_config.gridname, xmldata->sourcename, xmldata->hostname,
                                    getfield(host->strings, host->ip), "heartbeat", value, "int",
                                    "seconds", NULL, xmldata->source.localtime, getfield(host->strings, host->tags),
                                    getfield(host->strings, host->location), tmax * 4);

      if (event) {
         if (!strcmp(gmetad_config.riemann_protocol, "udp")) {
             send_event_to_riemann (event);
         } else {
             riemann_num_events++;
             riemann_msg->events = malloc (sizeof (Event) * riemann_num_events);
             riemann_msg->n_events = riemann_num_events;
             riemann_msg->events[riemann_num_events - 1] = event;
         }
      }
    }
#endif /* WITH_RIEMANN */

   /* Trim structure to the correct length. */
   hashval.size = sizeof(*host) - GMETAD_FRAMESIZE + host->stringslen;
   hashval.data = host;

   /* We dont care if this is an insert or an update. */
   rdatum = hash_insert(&hashkey, &hashval, hosts);
   if (!rdatum) {
         err_msg("Could not insert host %s", name);
         return 1;
   }
   return 0;
}


static int
startElement_HOSTS(void *data, const char *el, const char **attr)
{
   xmldata_t *xmldata = (xmldata_t *)data;
   int i;
   struct xml_tag *xt;

   /* In non-scalable mode, we do not process summary data. */
   if (!gmetad_config.scalable_mode) return 0;

   /* Add up/down hosts to this grid summary */
   for(i = 0; attr[i]; i+=2)
      {
         xt = in_xml_list (attr[i], strlen(attr[i]));
         if (!xt)
            continue;

         switch( xt->tag )
            {
               case UP_TAG:
                  xmldata->source.hosts_up += strtoul(attr[i+1], 
                                  (char **) NULL, 10);
                  break;
               case DOWN_TAG:
                  xmldata->source.hosts_down += strtoul(attr[i+1], 
                                  (char **) NULL, 10);
                  break;
               default:
                  break;
            }
      }
   return 0;
}


static int
startElement_METRIC(void *data, const char *el, const char **attr)
{
   xmldata_t *xmldata = (xmldata_t *)data;
   ganglia_slope_t slope = GANGLIA_SLOPE_UNSPECIFIED;
   struct xml_tag *xt;
   struct type_tag *tt;
   datum_t *hash_datum = NULL;
   datum_t *rdatum;
   datum_t hashkey, hashval;
   const char *name = NULL;
   const char *metricval = NULL;
   const char *type = NULL;
   const char *units = NULL;
   int do_summary;
   int i, edge, carbon_ret;
   hash_t *summary;
   Metric_t *metric;

   if (!xmldata->host_alive ) return 0;

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
               case UNITS_TAG:
                  units = attr[i+1];
                  break;
               case SLOPE_TAG:
                  slope = cstr_to_slope(attr[i+1]);
               default:
                  break;
            }
      }

   metric = &(xmldata->metric);
   memset((void*) metric, 0, sizeof(*metric));

   /* Summarize all numeric metrics */
   do_summary = 0;
   tt = in_type_list(type, strlen(type));
   if (!tt) return 0;

   if (tt->type==INT || tt->type==UINT || tt->type==FLOAT)
      do_summary = 1;

   /* Only keep metric details if we are the authority on this cluster. */
   if (authority_mode(xmldata))
      {
         /* Save the data to a round robin database if the data source is alive
          */
         fillmetric(attr, metric, type);
	 if (metric->dmax && metric->tn > metric->dmax)
            return 0;

#ifdef WITH_RIEMANN
         /* Forward all metrics, including strings, to Riemann */
        if (gmetad_config.riemann_server) {

            Host_t *host = (Host_t*) host;
            host = &(xmldata->host);
            Event *event;

            if (tt->type == INT || tt->type == UINT) {
               event = create_riemann_event (gmetad_config.gridname, xmldata->sourcename, xmldata->hostname,
                                              getfield(host->strings, host->ip), name, metricval, "int",
                                              units, NULL, xmldata->source.localtime, getfield(host->strings, host->tags),
                                              getfield(host->strings, host->location), metric->tmax);
            } else if (tt->type == FLOAT) {
               event = create_riemann_event (gmetad_config.gridname, xmldata->sourcename, xmldata->hostname,
                                              getfield(host->strings, host->ip), name, metricval, "float",
                                              units, NULL, xmldata->source.localtime, getfield(host->strings, host->tags),
                                              getfield(host->strings, host->location), metric->tmax);
            } else {
               event = create_riemann_event (gmetad_config.gridname, xmldata->sourcename, xmldata->hostname,
                                              getfield(host->strings, host->ip), name, metricval, "string",
                                              units, NULL, xmldata->source.localtime, getfield(host->strings, host->tags),
                                              getfield(host->strings, host->location), metric->tmax);
            }
            if (event) {
                if (!strcmp(gmetad_config.riemann_protocol, "udp")) {
                    send_event_to_riemann (event);
                } else {
                    riemann_num_events++;
                    riemann_msg->events = realloc (riemann_msg->events, sizeof (Event) * riemann_num_events);
                    riemann_msg->n_events = riemann_num_events;
                    riemann_msg->events[riemann_num_events - 1] = event;
                }
            }
        }
#endif /* WITH_RIEMANN */

         if (do_summary && !xmldata->ds->dead && !xmldata->rval)
            {
                  debug_msg("Updating host %s, metric %s", 
                                  xmldata->hostname, name);
                  if ( gmetad_config.write_rrds == 1 )
                     xmldata->rval = write_data_to_rrd(xmldata->sourcename,
                        xmldata->hostname, name, metricval, NULL,
                        xmldata->ds->step, xmldata->source.localtime, slope);
		  if (gmetad_config.carbon_server) // if the user has specified a carbon server, send the metric to carbon as well
                     carbon_ret=write_data_to_carbon(xmldata->sourcename, xmldata->hostname, name, metricval,xmldata->source.localtime);
#ifdef WITH_MEMCACHED
		  if (gmetad_config.memcached_parameters) {
                     int mc_ret=write_data_to_memcached(xmldata->sourcename, xmldata->hostname, name, metricval, xmldata->source.localtime, metric->dmax);
		  }
#endif /* WITH_MEMCACHED */
            }

         metric->id = METRIC_NODE;
         metric->report_start = metric_report_start;
         metric->report_end = metric_report_end;


         edge = metric->stringslen;
         metric->name = addstring(metric->strings, &edge, name);
         metric->stringslen = edge;

         /* Set local idea of T0. */
         metric->t0 = xmldata->now;
         metric->t0.tv_sec -= metric->tn;

         /* Trim metric structure to the correct length. */
         hashval.size = sizeof(*metric) - GMETAD_FRAMESIZE + metric->stringslen;
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
                     case UINT:
                     case FLOAT:
                        metric->val.d += (double)
                                strtod(metricval, (char**) NULL);
                        break;
                     default:
                        break;
                  }
            }

         metric->num++;
         metric->t0 = xmldata->now; /* tell cleanup thread we are using this */

         /* Trim metric structure to the correct length. Tricky. */
         hashval.size = sizeof(*metric) - GMETAD_FRAMESIZE + metric->stringslen;
         hashval.data = (void*) metric;

         /* Update metric in summary table. */
         rdatum = hash_insert(&hashkey, &hashval, summary);
         if (!rdatum) err_msg("Could not insert %s metric", name);
      }
   return 0;
}


static int
startElement_EXTRA_DATA(void *data, const char *el, const char **attr)
{
   return 0;
}

/* XXX - There is an issue which will cause a failure if there are more than 16
  EXTRA_ELEMENTs.  This is a problem with the size of the data structure that
  is used to hold the metric information.
*/
static int
startElement_EXTRA_ELEMENT (void *data, const char *el, const char **attr)
{
    xmldata_t *xmldata = (xmldata_t *)data;
    int edge;
    struct xml_tag *xt;
    int i, name_off, value_off;
    Metric_t metric;
    char *name = getfield(xmldata->metric.strings, xmldata->metric.name);
    datum_t *rdatum;
    datum_t hashkey, hashval;
    datum_t *hash_datum = NULL;
    
    if (!xmldata->host_alive) 
        return 0;

    /* Only keep extra element details if we are the authority on this cluster. */
    if (!authority_mode(xmldata))
       return 0;

    hashkey.data = (void*) name;
    hashkey.size =  strlen(name) + 1;
    
    hash_datum = hash_lookup (&hashkey, xmldata->host.metrics);
    if (!hash_datum) 
        return 0;
    
    memcpy(&metric, hash_datum->data, hash_datum->size);
    datum_free(hash_datum);

    /* Check to make sure that we don't try to add more
        extra elements than the array can handle.
    */  
    if (metric.ednameslen >= MAX_EXTRA_ELEMENTS) 
    {
        debug_msg("Can not add more extra elements for [%s].  Capacity of %d reached.",
                  name, MAX_EXTRA_ELEMENTS);
        return 0;
    }

    edge = metric.stringslen;
    
    name_off = value_off = -1;
    for(i = 0; attr[i]; i+=2)
    {
        xt = in_xml_list(attr[i], strlen(attr[i]));
        if (!xt) 
            continue;
        switch (xt->tag)
        {
        case NAME_TAG:
            name_off = i;
            break;
        case VAL_TAG:
            value_off = i;
            break;
        default:
            break;
        }
    }
    
    if ((name_off >= 0) && (value_off >= 0)) 
    {
        const char *new_name = attr[name_off+1];
        const char *new_value = attr[value_off+1];

        metric.ednames[metric.ednameslen++] = addstring(metric.strings, &edge, new_name);
        metric.edvalues[metric.edvalueslen++] = addstring(metric.strings, &edge, new_value);
    
        metric.stringslen = edge;
    
        hashkey.data = (void*)name;
        hashkey.size =  strlen(name) + 1;
    
        /* Trim metric structure to the correct length. */
        hashval.size = sizeof(metric) - GMETAD_FRAMESIZE + metric.stringslen;
        hashval.data = (void*) &metric;
    
        /* Update full metric in cluster host table. */
        rdatum = hash_insert(&hashkey, &hashval, xmldata->host.metrics);
        if (!rdatum)
        {
            err_msg("Could not insert %s metric", name);
        }
        else
        {
            hash_t *summary = xmldata->source.metric_summary;
            Metric_t sum_metric;

            /* do not add every SPOOF_HOST element to the summary table.
               if the same metric is SPOOF'd on more than ~MAX_EXTRA_ELEMENTS hosts
               then its summary table is destroyed.
             */
            if ( strlen(new_name) == 10 && !strcasecmp(new_name, SPOOF_HOST) )
                return 0;

            /* only update summary if metric is in hash */
            hash_datum = hash_lookup(&hashkey, summary);

            if (hash_datum) {
                int found = FALSE;

                memcpy(&sum_metric, hash_datum->data, hash_datum->size);
                datum_free(hash_datum);

                for (i = 0; i < sum_metric.ednameslen; i++) {
                    char *chk_name = getfield(sum_metric.strings, sum_metric.ednames[i]);
                    char *chk_value = getfield(sum_metric.strings, sum_metric.edvalues[i]);

                    /* If the name and value already exists, skip adding the strings. */
                    if (!strcasecmp(chk_name, new_name) && !strcasecmp(chk_value, new_value)) {
                        found = TRUE;
                        break;
                    }
                }
                if (!found) {
                    edge = sum_metric.stringslen;
                    sum_metric.ednames[sum_metric.ednameslen++] = addstring(sum_metric.strings, &edge, new_name);
                    sum_metric.edvalues[sum_metric.edvalueslen++] = addstring(sum_metric.strings, &edge, new_value);
                    sum_metric.stringslen = edge;
                }

                /* Trim graph display sum_metric at (352, 208) now or when in startElement_EXTRA_ELEMENT
metric structure to the correct length. Tricky. */
                hashval.size = sizeof(sum_metric) - GMETAD_FRAMESIZE + sum_metric.stringslen;
                hashval.data = (void*) &sum_metric;

                /* Update metric in summary table. */
                rdatum = hash_insert(&hashkey, &hashval, summary);
                if (!rdatum)
                    err_msg("Could not insert summary %s metric", name);
            }
        }
    }
    
    return 0;
}

static int
startElement_METRICS(void *data, const char *el, const char **attr)
{
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
   int i;
   hash_t *summary;
   Metric_t *metric;

   /* In non-scalable mode, we do not process summary data. */
   if (!gmetad_config.scalable_mode) return 0;
   
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
         if (!tt) return 0;
         switch (tt->type)
               {
                  case INT:
                  case UINT:
                  case FLOAT:
                     metric->val.d += (double)
                             strtod(metricval, (char**) NULL);
                     break;
                  default:
                     break;
               }
            metric->num += atoi(metricnum);
         }

   /* Update metric in summary table. */
   hashval.size = sizeof(*metric) - GMETAD_FRAMESIZE + metric->stringslen;
   hashval.data = (void*) metric;

   summary = xmldata->source.metric_summary;
   rdatum = hash_insert(&hashkey, &hashval, summary);
   if (!rdatum) {
      err_msg("Could not insert %s metric", name);
      return 1;
   }
   return 0;
}


static int
startElement_GANGLIA_XML(void *data, const char *el, const char **attr)
{
   xmldata_t *xmldata = (xmldata_t *)data;
   struct xml_tag *xt;
   int i;

   for(i = 0; attr[i] ; i+=2)
      {
         /* Only process the XML tags that gmetad is interested in */
         if( !( xt = in_xml_list ( (char *)attr[i], strlen(attr[i]))) )
            continue;

         if (xt->tag == VERSION_TAG)
            {
                  /* Process the version tag later */
                  if( strncmp( attr[i+1], "2.5" , 3 ) < 0 )
                     {
                         debug_msg("[%s] is a pre-2.5 data stream", xmldata->ds->name);
                         xmldata->old = 1;
                      }
                  else
                     {
                         debug_msg("[%s] is a 2.5 or later data stream", xmldata->ds->name);
                         xmldata->old = 0;
                      }
             }
       } 
   return 0;
}


/* Called when a start tag is encountered.
 * sacerdoti: Move real processing to smaller functions for 
 * maintainability.
 */
static void
start (void *data, const char *el, const char **attr)
{
   struct xml_tag *xt;
   int rc;

   xt = in_xml_list ((char *) el, strlen(el));
   if (!xt)
      return;

   switch( xt->tag )
      {
         case GRID_TAG:
            rc = startElement_GRID(data, el, attr);
            break;

         case CLUSTER_TAG:
            rc = startElement_CLUSTER(data, el, attr);
            break;

         case HOST_TAG:
            rc = startElement_HOST(data, el, attr);
            break;

         case HOSTS_TAG:
            rc = startElement_HOSTS(data, el, attr);
            break;
               
         case METRIC_TAG:
            rc = startElement_METRIC(data, el, attr);
            break;

         case METRICS_TAG:
            rc = startElement_METRICS(data, el, attr);
            break;

         case GANGLIA_XML_TAG:
            rc = startElement_GANGLIA_XML(data, el, attr);
            break;

         case EXTRA_DATA_TAG:
            rc = startElement_EXTRA_DATA(data, el, attr);
            break;

         case EXTRA_ELEMENT_TAG:
            rc = startElement_EXTRA_ELEMENT(data, el, attr);
            break;
    
         default:
            break;
      }  /* end switch */

   return;
}



/* Write a metric summary value to the RRD database. */
static int
finish_processing_source(datum_t *key, datum_t *val, void *arg)
{
   xmldata_t *xmldata = (xmldata_t *) arg;
   char *name, *type;
   char sum[512];
   char num[256];
   Metric_t *metric;
   struct type_tag *tt;
   llist_entry *le;
   char *p;

   name = (char*) key->data;
   metric = (Metric_t*) val->data;
   type = getfield(metric->strings, metric->type);

   /* Don't save to RRD if the datasource is dead or write_rrds is off */
   if( xmldata->ds->dead || gmetad_config.write_rrds != 1)
      return 1;

   tt = in_type_list(type, strlen(type));
   if (!tt) return 0;

   /* Don't save to RRD if this is a metric not to be summarized */
   if (llist_search(&(gmetad_config.unsummarized_metrics), (void *)name, llist_strncmp, &le) == 0)
      return 0;

   /* Don't save to RRD if this metrics appears to be an sFlow VM metrics */
   if (gmetad_config.unsummarized_sflow_vm_metrics && (p = strchr(name, '.')) != NULL && *(p+1) == 'v')
       return 0;

   switch (tt->type)
      {
         case INT:
         case UINT:
            sprintf(sum, "%.f", metric->val.d);
            break;
         case FLOAT:
            sprintf(sum, "%.*f", (int) metric->precision, metric->val.d);
            break;
         default:
            break;
      }
   sprintf(num, "%u", metric->num);

   /* Save the data to a round robin database if this data source is 
    * alive. */
   if (!xmldata->ds->dead && !xmldata->rval)
     {

       debug_msg("Writing Summary data for source %s, metric %s",
                 xmldata->sourcename, name);

       xmldata->rval = write_data_to_rrd(xmldata->sourcename, NULL, name,
                                         sum, num, xmldata->ds->step,
                                         xmldata->source.localtime,
                                         cstr_to_slope(getfield(metric->strings, metric->slope)));
     }

   return xmldata->rval;
}


static int
endElement_GRID(void *data, const char *el)
{
   xmldata_t *xmldata = (xmldata_t *) data;

   /* In non-scalable mode, we ignore GRIDs. */
   if (!gmetad_config.scalable_mode)
      return 0;

   xmldata->grid_depth--;
   debug_msg("Found a </GRID>, depth is now %d", xmldata->grid_depth);

   datum_t hashkey, hashval;
   datum_t *rdatum;
   hash_t *summary;
   Source_t *source;

   /* Only keep info on sources we are an authority on. */
   if (authority_mode(xmldata))
      {
         source = &xmldata->source;
         summary = xmldata->source.metric_summary;

         /* Release the partial sum mutex */
         pthread_mutex_unlock(source->sum_finished);

         hashkey.data = (void*) xmldata->sourcename;
         hashkey.size = strlen(xmldata->sourcename) + 1;

         hashval.data = source;
         /* Trim structure to the correct length. */
         hashval.size = sizeof(*source) - GMETAD_FRAMESIZE + source->stringslen;

         /* We insert here to get an accurate hosts up/down value. */
         rdatum = hash_insert( &hashkey, &hashval, xmldata->root);
         if (!rdatum) {
               err_msg("Could not insert source %s", xmldata->sourcename);
               return 1;
         }
         /* Write the metric summaries to the RRD. */
         hash_foreach(summary, finish_processing_source, data);
      }
   return 0;
}


static int
endElement_CLUSTER(void *data, const char *el)
{
   xmldata_t *xmldata = (xmldata_t *) data;
   datum_t hashkey, hashval;
   datum_t *rdatum;
   hash_t *summary;
   Source_t *source;

   /* Only keep info on sources we are an authority on. */
   if (authority_mode(xmldata))
      {
         source = &xmldata->source;
         summary = xmldata->source.metric_summary;

         /* Release the partial sum mutex */
         pthread_mutex_unlock(source->sum_finished);

         hashkey.data = (void*) xmldata->sourcename;
         hashkey.size = strlen(xmldata->sourcename) + 1;

         hashval.data = source;
         /* Trim structure to the correct length. */
         hashval.size = sizeof(*source) - GMETAD_FRAMESIZE + source->stringslen;

         /* We insert here to get an accurate hosts up/down value. */
         rdatum = hash_insert( &hashkey, &hashval, xmldata->root);
         if (!rdatum) {
               err_msg("Could not insert source %s", xmldata->sourcename);
               return 1;
         }
         /* Write the metric summaries to the RRD. */
         hash_foreach(summary, finish_processing_source, data);
      }
   return 0;
}

#ifdef WITH_RIEMANN
static int
endElement_HOST(void *data, const char *el)
{
   if (!strcmp(gmetad_config.riemann_protocol, "tcp")) {
      send_message_to_riemann(riemann_msg);
      destroy_riemann_msg(riemann_msg);
      riemann_num_events = 0;
   }
   return 0;
}
#endif /* WITH_RIEMANN */

static void
end (void *data, const char *el)
{
   struct xml_tag *xt;
   int rc;

   if(! (xt = in_xml_list((char*) el, strlen(el))) )
      return;

   switch ( xt->tag )
      {
         case GRID_TAG:
            rc = endElement_GRID(data, el);
            break;

         case CLUSTER_TAG:
            rc = endElement_CLUSTER(data, el);
            break;

#ifdef WITH_RIEMANN
         case HOST_TAG:
            rc = endElement_HOST(data, el);
            break;
#endif /* WITH_RIEMANN */

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

   memset( &xmldata, 0, sizeof( xmldata ));

   /* Set the pointer to the data source record */
   xmldata.ds = d;
   
   /* Set the hash table for the root data source. */
   xmldata.root = root.authority;
   
   gettimeofday(&xmldata.now, NULL);

   xml_parser = XML_ParserCreate (NULL);
   if (! xml_parser)
      {
         err_msg("Process XML: unable to create XML parser");
         return 1;
      }

   XML_SetElementHandler (xml_parser, start, end);
   XML_SetUserData (xml_parser, &xmldata);

   rval = XML_Parse( xml_parser, buf, strlen(buf), 1 );
   if(! rval )
      {
         err_msg ("Process XML (%s): XML_ParseBuffer() error at line %d:\n%s\n",
                         d->name,
                         (int) XML_GetCurrentLineNumber (xml_parser),
                         XML_ErrorString (XML_GetErrorCode (xml_parser)));
         xmldata.rval = 1;
      }

   /* Free memory that might have been allocated in xmldata */
   if (xmldata.sourcename)
      free(xmldata.sourcename);

   if (xmldata.hostname)
      free(xmldata.hostname);

   XML_ParserFree(xml_parser);
   return xmldata.rval;
}

