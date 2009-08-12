<?php
/* $Id$ */

include_once "./functions.php";
  
$meta_designator = "Grid";
$cluster_designator = "Cluster Overview";

# Blocking malicious CGI input.
$clustername = isset($_GET["c"]) ?
    escapeshellcmd( clean_string( rawurldecode($_GET["c"]) ) ) : NULL;
$gridname = isset($_GET["G"]) ?
    escapeshellcmd( clean_string( rawurldecode($_GET["G"]) ) ) : NULL;
if($case_sensitive_hostnames == 1) {
    $hostname = isset($_GET["h"]) ?
        escapeshellcmd( clean_string( rawurldecode($_GET["h"]) ) ) : NULL;
} else {
    $hostname = isset($_GET["h"]) ?
        strtolower( escapeshellcmd( clean_string( rawurldecode($_GET["h"]) ) ) ) : NULL;
}
$range = isset( $_GET["r"] ) && in_array($_GET["r"], array_keys( $time_ranges ) ) ?
    escapeshellcmd( rawurldecode($_GET["r"])) : NULL;
$metricname = isset($_GET["m"]) ?
    escapeshellcmd( clean_string( rawurldecode($_GET["m"]) ) ) : NULL;
$metrictitle = isset($_GET["ti"]) ?
    escapeshellcmd( clean_string( rawurldecode($_GET["ti"]) ) ) : NULL;
$sort = isset($_GET["s"]) ?
    escapeshellcmd( clean_string( rawurldecode($_GET["s"]) ) ) : NULL;
$controlroom = isset($_GET["cr"]) ?
    clean_number( rawurldecode($_GET["cr"]) ) : NULL;
# Default value set in conf.php, Allow URL to overrride
if (isset($_GET["hc"]))
    $hostcols = clean_number($_GET["hc"]);
if (isset($_GET["mc"]))
    $metriccols = clean_number($_GET["mc"]);
# Flag, whether or not to show a list of hosts
$showhosts = isset($_GET["sh"]) ?
    clean_number( $_GET["sh"] ) : NULL;
# The 'p' variable specifies the verbosity level in the physical view.
$physical = isset($_GET["p"]) ?
    clean_number( $_GET["p"] ) : NULL;
$tree = isset($_GET["t"]) ?
    escapeshellcmd($_GET["t"] ) : NULL;
# A custom range value for job graphs, in -sec.
$jobrange = isset($_GET["jr"]) ?
    clean_number( $_GET["jr"] ) : NULL;
# A red vertical line for various events. Value specifies the event time.
$jobstart = isset($_GET["js"]) ?
    clean_number( $_GET["js"] ) : NULL;
# custom start and end
$cs = isset($_GET["cs"]) ?
    escapeshellcmd($_GET["cs"]) : NULL;
$ce = isset($_GET["ce"]) ?
    escapeshellcmd($_GET["ce"]) : NULL;
# The direction we are travelling in the grid tree
$gridwalk = isset($_GET["gw"]) ?
    escapeshellcmd( clean_string( $_GET["gw"] ) ) : NULL;
# Size of the host graphs in the cluster view
$clustergraphsize = isset($_GET["z"]) && in_array( $_GET[ 'z' ], $graph_sizes_keys ) ?
    escapeshellcmd($_GET["z"]) : NULL;
# A stack of grid parents. Prefer a GET variable, default to cookie.
if (isset($_GET["gs"]) and $_GET["gs"])
    $gridstack = explode( ">", rawurldecode( $_GET["gs"] ) );
else if ( isset($_COOKIE['gs']) and $_COOKIE['gs'])
    $gridstack = explode( ">", $_COOKIE["gs"] );

if (isset($gridstack) and $gridstack) {
   foreach( $gridstack as $key=>$value )
      $gridstack[ $key ] = clean_string( $value );
}

# Assume we are the first grid visited in the tree if there is no gridwalk
# or gridstack is not well formed. Gridstack always has at least one element.
if ( !isset($gridstack) or !strstr($gridstack[0], "http://"))
    $initgrid = TRUE;

# Default values
if (!isset($hostcols) || !is_numeric($hostcols)) $hostcols = 4;
if (!isset($metriccols) || !is_numeric($metriccols)) $metriccols = 2;
if (!is_numeric($showhosts)) $showhosts = 1;

# Filters
if(isset($_GET["choose_filter"]))
{
  $req_choose_filter = $_GET["choose_filter"];
  $choose_filter = array();
  foreach($req_choose_filter as $k_req => $v_req)
  {
    $k = escapeshellcmd( clean_string( rawurldecode ($k_req)));
    $v = escapeshellcmd( clean_string( rawurldecode ($v_req)));
    $choose_filter[$k] = $v;
  }
}

# Set context.
if(!$clustername && !$hostname && $controlroom)
   {
      $context = "control";
   }
else if (isset($tree))
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
    $range = "$default_time_range";

$end = "N";

# $time_ranges defined in conf.php
if( $range == 'job' && isSet( $jobrange ) ) {
    $start = $jobrange;
} else if( isSet( $time_ranges[ $range ] ) ) {
    $start = $time_ranges[ $range ] * -1;
} else {
    $start = $time_ranges[ $default_time_range ] * -1;
}

if ($cs or $ce)
    $range = "custom";

if (!$metricname)
    $metricname = "$default_metric";

if (!$sort)
    $sort = "descending";

# Since cluster context do not have the option to sort "by hosts down" or
# "by hosts up", therefore change sort order to "descending" if previous
# sort order is either "by hosts down" or "by hosts up"
if ($context == "cluster") {
    if ($sort == "by hosts up" || $sort == "by hosts down") {
        $sort = "descending";
    }
}

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
