/* $Id$ */
#include <stdio.h>
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

static void
start (void *data, const char *el, const char **attr)
{
  int i, is_string, is_constant, is_stale, tn, tmax;

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
                    if(! strcmp( attr[i+1], "string" ))
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
              if( push_data_to_rrd( cluster, host, metric, metric_val) )
                 {
                    /* Pass the error on to save_to_rrd */
                    *(int *)data = 1;
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
  if(! strcmp("CLUSTER", el) )
     {
        
     }
  else if(! strcmp("HOST", el) )
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
   int rval, func_rval = 0;
   XML_Parser xml_parser;

   xml_parser = XML_ParserCreate (NULL);
   if (! xml_parser)
      {
         err_msg("save_to_rrd() unable to create XML parser");
         return 1;
      }

   XML_SetElementHandler (xml_parser, start, end);
   XML_SetUserData       (xml_parser, &func_rval);

   rval = XML_Parse( xml_parser, buf, strlen(buf), 1 );
   if(! rval )
      {
         err_msg ("save_to_rrd() XML_ParseBuffer() error at line %d:\n%s\n",
                         XML_GetCurrentLineNumber (xml_parser),
                         XML_ErrorString (XML_GetErrorCode (xml_parser)));
         return 1;
      }

   XML_ParserFree(xml_parser);
   return func_rval;
}
