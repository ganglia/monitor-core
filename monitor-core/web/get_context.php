<?php
/* $Id$ */

$meta_designator = "Grid";
$cluster_designator = "Cluster Overview";

# Blocking malicious CGI input.
$clustername = isset($_GET["c"]) ?
	escapeshellcmd(rawurldecode($_GET["c"])) : NULL;
$gridname = isset($_GET["G"]) ?
	escapeshellcmd(rawurldecode($_GET["G"])) : NULL;
$hostname = isset($_GET["h"]) ?
	escapeshellcmd(rawurldecode($_GET["h"])) : NULL;
$range = isset($_GET["r"]) ?
	escapeshellcmd(rawurldecode($_GET["r"])) : NULL;
$metricname = isset($_GET["m"]) ?
	escapeshellcmd(rawurldecode($_GET["m"])) : NULL;
$sort = isset($_GET["s"]) ?
	escapeshellcmd(rawurldecode($_GET["s"])) : NULL;
$controlroom = isset($_GET["cr"]) ?
	escapeshellcmd(rawurldecode($_GET["cr"])) : NULL;
$hostcols = isset($_GET["hc"]) ?
	escapeshellcmd($_GET["hc"]) : NULL;
$showhosts = isset($_GET["sh"]) ?
	escapeshellcmd($_GET["sh"]) : NULL;
# The 'p' variable specifies the verbosity level in the physical view.
$physical = isset($_GET["p"]) ?
	escapeshellcmd($_GET["p"]) : NULL;
$tree = isset($_GET["t"]) ?
	escapeshellcmd($_GET["t"] ) : NULL;
# A custom range value for job graphs, in -sec.
$jobrange = isset($_GET["jr"]) ?
	escapeshellcmd($_GET["jr"]) : NULL;
# A red vertical line for various events. Value specifies the event time.
$jobstart = isset($_GET["js"]) ?
	escapeshellcmd($_GET["js"]) : NULL;
# The direction we are travelling in the grid tree
$gridwalk = isset($_GET["gw"]) ?
	escapeshellcmd($_GET["gw"]) : NULL;
# A stack of grid parents. Prefer a GET variable, default to cookie.
if (isset($_GET["gs"]) and $_GET["gs"])
      $gridstack = explode(">", rawurldecode($_GET["gs"]));
else
      $gridstack = explode(">", $_COOKIE["gs"] );

# Assume we are the first grid visited in the tree if there are no CGI variables,
# or gridstack is not well formed. Gridstack always has at least one element.
if (!count($_GET) or !strstr($gridstack[0], "http://"))
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
