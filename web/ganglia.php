<?php
/* $Id$ */
#
# Parses ganglia XML tree.
#
# The arrays defined in the first part of this file to hold XML info. 
#
# sacerdoti: These are now context-sensitive, and hold only as much
# information as we need to make the page.
#

$error="";

# Gives time in seconds to retrieve and parse XML tree. With subtree-
# capable gmetad, should be very fast in all but the largest cluster configurations.
$parsetime = 0;

# 2key = "Source Name" / "NAME | AUTHORITY | HOSTS_UP ..." = Value.
$grid = array();

# 1Key = "NAME | LOCALTIME | HOSTS_UP | HOSTS_DOWN" = Value.
$cluster = array();

# 2Key = "Cluster Name / Host Name" ... Value = Array of Host Attributes
$hosts_up = array();
# 2Key = "Cluster Name / Host Name" ... Value = Array of Host Attributes
$hosts_down = array();

# Context dependant structure.
$metrics = array();

# 1Key = "Component" (gmetad | gmond) = Version string
$version = array();

# The web frontend version, from conf.php.
#$version["webfrontend"] = "$majorversion.$minorversion.$microversion";
$version["webfrontend"] = "$ganglia_version";

# Get rrdtool version
$rrdtool_version = array();
exec(RRDTOOL, $rrdtool_version);
$rrdtool_version = explode(" ", $rrdtool_version[0]);
$rrdtool_version = $rrdtool_version[1];
$version["rrdtool"] = "$rrdtool_version";
 
# The name of our local grid.
$self = " ";


# Returns true if the host is alive. Works for both old and new gmond sources.
function host_alive($host, $cluster)
{
   $TTL = 60;

   if ($host['TN'] and $host['TMAX']) {
      if ($host['TN'] > $host['TMAX'] * 4)
         return FALSE;
         $host_up = FALSE;
   }
   else {      # The old method.
      if (abs($cluster["LOCALTIME"] - $host['REPORTED']) > (4*$TTL))
         return FALSE;
   }
   return TRUE;
}


# Called with <GANGLIA_XML> attributes.
function preamble($ganglia)
{
   global $version;

   $component = $ganglia['SOURCE'];
   $version[$component] = $ganglia['VERSION'];
}


function start_meta ($parser, $tagname, $attrs)
{
   global $metrics, $grid, $self;
   static $sourcename, $metricname;

   switch ($tagname)
      {
         case "GANGLIA_XML":
            preamble($attrs);
            break;

         case "GRID":
         case "CLUSTER":
            # Our grid will be first.
            if (!$sourcename) $self = $attrs['NAME'];

            $sourcename = $attrs['NAME'];
            $grid[$sourcename] = $attrs;

            # Identify a grid from a cluster.
            $grid[$sourcename][$tagname] = 1;
            break;

         case "METRICS":
            $metricname = $attrs['NAME'];
            $metrics[$sourcename][$metricname] = $attrs;
            break;

         case "HOSTS":
            $grid[$sourcename]['HOSTS_UP'] = $attrs['UP'];
            $grid[$sourcename]['HOSTS_DOWN'] = $attrs['DOWN'];
            break;

         default:
            break;
      }
}


function start_cluster ($parser, $tagname, $attrs)
{
   global $metrics, $cluster, $self, $grid, $hosts_up, $hosts_down;
   static $hostname;

   switch ($tagname)
      {
         case "GANGLIA_XML":
            preamble($attrs);
            break;
         case "GRID":
            $self = $attrs['NAME'];
            $grid = $attrs;
            break;

         case "CLUSTER":
            $cluster = $attrs;
            break;

         case "HOST":
            $hostname = $attrs['NAME'];

            if (host_alive($attrs, $cluster))
               {
		  isset($cluster['HOSTS_UP']) or $cluster['HOSTS_UP'] = 0;
                  $cluster['HOSTS_UP']++;
                  $hosts_up[$hostname] = $attrs;
               }
            else
               {
		  isset($cluster['HOSTS_DOWN']) or $cluster['HOSTS_DOWN'] = 0;
                  $cluster['HOSTS_DOWN']++;
                  $hosts_down[$hostname] = $attrs;
               }
            break;

         case "METRIC":
            $metricname = $attrs['NAME'];
            $metrics[$hostname][$metricname] = $attrs;
            break;

         default:
            break;
      }
}


function start_cluster_summary ($parser, $tagname, $attrs)
{
   global $metrics, $cluster, $self, $grid;

   switch ($tagname)
      {
         case "GANGLIA_XML":
            preamble($attrs);
            break;
         case "GRID":
            $self = $attrs['NAME'];
            $grid = $attrs;
         case "CLUSTER":
            $cluster = $attrs;
            break;
         
         case "HOSTS":
            $cluster['HOSTS_UP'] = $attrs['UP'];
            $cluster['HOSTS_DOWN'] = $attrs['DOWN'];
            break;
            
         case "METRICS":
            $metrics[$attrs['NAME']] = $attrs;
            break;
            
         default:
            break;
      }
}


function start_host ($parser, $tagname, $attrs)
{
   global $metrics, $cluster, $hosts_up, $hosts_down, $self, $grid;

   switch ($tagname)
      {
         case "GANGLIA_XML":
            preamble($attrs);
            break;
         case "GRID":
            $self = $attrs['NAME'];
            $grid = $attrs;
            break;
         case "CLUSTER":
            $cluster = $attrs;
            break;

         case "HOST":
            if (host_alive($attrs, $cluster))
               $hosts_up = $attrs;
            else
               $hosts_down = $attrs;
            break;

         case "METRIC":
            $metrics[$attrs['NAME']] = $attrs;
            break;

         default:
            break;
      }
}


function end_all ($parser, $tagname)
{

}


function Gmetad ()
{
   global $error, $parsetime, $clustername, $hostname, $context;
   # From conf.php:
   global $ganglia_ip, $ganglia_port;

   # Parameters are optionalshow
   # Defaults...
   $ip = $ganglia_ip;
   $port = $ganglia_port;
   $timeout = 3.0;
   $errstr = "";
   $errno  = "";

   switch( func_num_args() )
      {
         case 2:
            $port = func_get_arg(1);
         case 1:
            $ip = func_get_arg(0);
      }

   $parser = xml_parser_create();
   switch ($context)
      {
         case "meta":
         case "control":
         case "tree":
         default:
            xml_set_element_handler($parser, "start_meta", "end_all");
            $request = "/?filter=summary";
            break;
         case "physical":
         case "cluster":
            xml_set_element_handler($parser, "start_cluster", "end_all");
            $request = "/$clustername";
             break;
         case "cluster-summary":
            xml_set_element_handler($parser, "start_cluster_summary", "end_all");
            $request = "/$clustername?filter=summary";
            break;
         case "node":
         case "host":
            xml_set_element_handler($parser, "start_host", "end_all");
            $request = "/$clustername/$hostname";
            break;
      }

  $fp = fsockopen( $ip, $port, $errno, $errstr, $timeout);
   if (!$fp)
      {
         $error = "fsockopen error: $errstr";
         return FALSE;
      }

   if ($port == 8649)
      {
         # We are connecting to a gmond. Non-interactive.
         xml_set_element_handler($parser, "start_cluster", "end_all");
      }
   else
      {
         $request .= "\n";
         $rc = fputs($fp, $request);
         if (!$rc)
            {
               $error = "Could not sent request to gmetad: $errstr";
               return FALSE;
            }
      }

   $start = gettimeofday();

   while(!feof($fp))
      {
         $data = fread($fp, 16384);
         if (!xml_parse($parser, $data, feof($fp)))
            {
               $error = sprintf("XML error: %s at %d",
                  xml_error_string(xml_get_error_code($parser)),
                  xml_get_current_line_number($parser));
               fclose($fp);
               return FALSE;
            }
      }
   fclose($fp);

   $end = gettimeofday();
   $parsetime = ($end['sec'] + $end['usec']/1e6) - ($start['sec'] + $start['usec']/1e6);

   return TRUE;
}

?>
