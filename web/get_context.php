<?php
/* $Id$ */

$meta_designator = "Grid";
$cluster_designator = "Cluster Overview";

# Blocking malicious CGI input.
$clustername = escapeshellcmd(rawurldecode($HTTP_GET_VARS["c"]));
$gridname = escapeshellcmd(rawurldecode($HTTP_GET_VARS["G"]));
$hostname = escapeshellcmd(rawurldecode($HTTP_GET_VARS["h"]));
$range = escapeshellcmd(rawurldecode($HTTP_GET_VARS["r"]));
$metricname = escapeshellcmd(rawurldecode($HTTP_GET_VARS["m"]));
$sort = escapeshellcmd(rawurldecode($HTTP_GET_VARS["s"]));
$controlroom = escapeshellcmd(rawurldecode($HTTP_GET_VARS["cr"]));
$hostcols = escapeshellcmd($HTTP_GET_VARS["hc"]);
$showhosts = escapeshellcmd($HTTP_GET_VARS["sh"]);
# The 'p' variable specifies the verbosity level in the physical view.
$physical = escapeshellcmd($HTTP_GET_VARS["p"]);
$tree = escapeshellcmd($HTTP_GET_VARS["t"] );
# A custom range value for job graphs, in -sec.
$jobrange = escapeshellcmd($HTTP_GET_VARS["jr"]);
# A red vertical line for various events. Value specifies the event time.
$jobstart = escapeshellcmd($HTTP_GET_VARS["js"]);
# The direction we are travelling in the grid tree
$gridwalk = escapeshellcmd($HTTP_GET_VARS["gw"]);
# A stack of grid parents. Prefer a GET variable, default to cookie.
if ($HTTP_GET_VARS["gs"])
      $gridstack = explode(">", rawurldecode($HTTP_GET_VARS["gs"]));
else
      $gridstack = explode(">", $HTTP_COOKIE_VARS["gs"] );
# Assume we are the first grid visited in the tree if there are no CGI variables,
# or gridstack is not well formed. Gridstack always has at least one element.
if (!count($HTTP_GET_VARS) or !strstr($gridstack[0], "http://"))
      $initgrid=TRUE;

# Default values
if (!is_numeric($hostcols)) $hostcols = 4;
if (!is_numeric($showhosts)) $showhosts = 1;

# Set context.
if(!$clustername && !$hostname && $controlroom)
   {
      $context = "control";
   }
else if ($tree)
   {
      $context = "tree";
   }
else if(!$clustername and !$gridname and !$hostname)
   {
      $context = "meta";
   }
else if($gridname)
   {
      $context = "grid";
   }
else if ($clustername and !$hostname and $physical)
   {
      $context = "physical";
   }
else if ($clustername and !$hostname and !$showhosts)
   {
      $context = "cluster-summary";
   }
else if($clustername and !$hostname)
   {
      $context = "cluster";
   }
else if($clustername and $hostname and $physical)
   {
      $context = "node";
   }
else if($clustername and $hostname)
   {
      $context = "host";
   }

if (!$range)
      $range = "$default_range";

$end = "N";

switch ($range)
{
   case "hour":  $start = -3600; break;
   case "day":   $start = -86400; break;
   case "week":  $start = -604800; break;
   case "month": $start = -2419200; break;
   case "year":  $start = -31449600; break;
   case "job":
      if ($jobrange) {
        $start = $jobrange;
        break;
     }
   default: $start = -3600;
}

if (!$metricname)
      $metricname = "$default_metric";

if (!$sort)
      $sort = "descending";

# A hack for pre-2.5.0 ganglia data sources.
$always_constant = array(
   "swap_total" => 1,
   "cpu_speed" => 1,
   "swap_total" => 1
);

$always_timestamp = array(
   "gmond_started" => 1,
   "reported" => 1,
   "sys_clock" => 1,
   "boottime" => 1
);

# List of report graphs
$reports = array(
   "load_report" => "load_one",
   "cpu_report" => 1,
   "mem_report" => 1,
   "network_report" => 1,
   "packet_report" => 1
);

?>
