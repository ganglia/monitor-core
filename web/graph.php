<?php
/* $Id$ */
include_once "./eval_config.php";
include_once "./get_context.php";
include_once "./functions.php";


$ganglia_dir = dirname(__FILE__);

# RFM - Added all the isset() tests to eliminate "undefined index"
# messages in ssl_error_log.

# Graph specific variables
# ATD - No need for escapeshellcmd or rawurldecode on $size or $graph.  Not used directly in rrdtool calls.
$size = isset($_GET["z"]) && in_array( $_GET[ 'z' ], $graph_sizes_keys )
             ? $_GET["z"]
             : NULL;

# ATD - TODO, should encapsulate these custom graphs in some type of container, then this code could check list of defined containers for valid graph labels.
$graph      = isset($_GET["g"])  ?  sanitize ( $_GET["g"] )   : NULL;
$grid       = isset($_GET["G"])  ?  sanitize ( $_GET["G"] )   : NULL;
$self       = isset($_GET["me"]) ?  sanitize ( $_GET["me"] )  : NULL;
$vlabel     = isset($_GET["vl"]) ?  sanitize ( $_GET["vl"] )  : NULL;
$value      = isset($_GET["v"])  ?  sanitize ( $_GET["v"] )   : NULL;


$metric_name = isset($_GET["m"])  ?  sanitize ( $_GET["m"] )   : NULL;

$max        = isset($_GET["x"])  ?  clean_number ( sanitize ($_GET["x"] ) ) : NULL;
$min        = isset($_GET["n"])  ?  clean_number ( sanitize ($_GET["n"] ) ) : NULL;
$sourcetime = isset($_GET["st"]) ?  clean_number ( sanitize( $_GET["st"] ) ) : NULL;

$load_color = isset($_GET["l"]) && is_valid_hex_color( rawurldecode( $_GET[ 'l' ] ) )
                                 ?  sanitize ( $_GET["l"] )   : NULL;

$summary    = isset( $_GET["su"] )    ? 1 : 0;
$debug      = isset( $_GET['debug'] ) ? clean_number ( sanitize( $_GET["debug"] ) ) : 0;
$command    = '';

// Get hostname
$raw_host = isset($_GET["h"])  ?  sanitize ( $_GET["h"]  )   : "__SummaryInfo__";  

// For graphite purposes we need to replace all dots with underscore. dot  is
// separates subtrees in graphite
$host = str_replace(".","_", $raw_host);

# Assumes we have a $start variable (set in get_context.php).
# $graph_sizes and $graph_sizes_keys defined in conf.php.  Add custom sizes there.

$size = in_array( $size, $graph_sizes_keys ) ? $size : 'default';

if ( isset($_GET['height'] ) ) 
  $height = $_GET['height'];
else 
  $height  = $graph_sizes[ $size ][ 'height' ];

if ( isset($_GET['width'] ) ) 
  $width =  $_GET['width'];
else
  $width = $graph_sizes[ $size ][ 'width' ];

#$height  = $graph_sizes[ $size ][ 'height' ];
#$width   = $graph_sizes[ $size ][ 'width' ];
$fudge_0 = $graph_sizes[ $size ][ 'fudge_0' ];
$fudge_1 = $graph_sizes[ $size ][ 'fudge_1' ];
$fudge_2 = $graph_sizes[ $size ][ 'fudge_2' ];


/* ------------------------------------------------------------------------
  Graphite stuff
------------------------------------------------------------------------- */

function create_graphite_target_string($host_cluster, $array) {

  global $host;
  foreach ( $array as $metric => $metric_description ) {
    $elements[] = "target=alias($host_cluster." . $metric . ".sum%2C'" . urlencode($metric_description) . "')";
  }

  return join("&", $elements);

}



#
# Since the $command variable is explicitly set to an empty string, above, do we really need
# this check anymore?  --jb Jan 2008
#
# This security fix was brought to my attention by Peter Vreugdenhil <petervre@sci.kun.nl>
# Dont want users specifying their own malicious command via GET variables e.g.
# http://ganglia.mrcluster.org/graph.php?graph=blob&command=whoami;cat%20/etc/passwd
#
if($command) {
    error_log("Command variable sent, exiting!");
    exit();
}

switch ($context)
{
    case "meta":
      $rrd_dir = "$rrds/__SummaryInfo__";
      $rrd_graphite_link = "$graphite_rrd_dir/__SummaryInfo__";
      break;
    case "grid":
      $rrd_dir = "$rrds/$grid/__SummaryInfo__";
      $rrd_graphite_link = "$graphite_rrd_dir/$grid/__SummaryInfo__";
      break;
    case "cluster":
      $rrd_dir = "$rrds/$clustername/__SummaryInfo__";
      $rrd_graphite_link = "$graphite_rrd_dir/$clustername/__SummaryInfo__";
      break;
    case "host":
      $rrd_dir = "$rrds/$clustername/$raw_host";
      $rrd_graphite_link = $graphite_rrd_dir . "/" . $clustername . "/" . $host;
      break;
    default:
      exit;
}

if ($cs)
    $start = $cs;
if ($ce)
    $end = $ce;

# Set some standard defaults that don't need to change much
$rrdtool_graph = array(
    'start'  => $start,
    'end'    => $end,
    'width'  => $width,
    'height' => $height,
);

# automatically strip domainname from small graphs where it won't fit
if ($size == "small") {
    $strip_domainname = true;
    # Let load coloring work for little reports in the host list.
    if (! isset($subtitle) and $load_color)
        $rrdtool_graph['color'] = "BACK#'$load_color'";
}

if ($debug) {
    error_log("Graph [$graph] in context [$context]");
}

/* If we have $graph, then a specific report was requested, such as "network_report" or
 * "cpu_report.  These graphs usually have some special logic and custom handling required,
 * instead of simply plotting a single metric.  If $graph is not set, then we are (hopefully),
 * plotting a single metric, and will use the commands in the metric.php file.
 *
 * With modular graphs, we look for a "${graph}.php" file, and if it exists, we
 * source it, and call a pre-defined function name.  The current scheme for the function
 * names is:   'graph_' + <name_of_report>.  So a 'cpu_report' would call graph_cpu_report(),
 * which would be found in the cpu_report.php file.
 *
 * These functions take the $rrdtool_graph array as an argument.  This variable is
 * PASSED BY REFERENCE, and will be modified by the various functions.  Each key/value
 * pair represents an option/argument, as passed to the rrdtool program.  Thus,
 * $rrdtool_graph['title'] will refer to the --title option for rrdtool, and pass the array
 * value accordingly.
 *
 * There are two exceptions to:  the 'extras' and 'series' keys in $rrdtool_graph.  These are
 * assigned to $extras and $series respectively, and are treated specially.  $series will contain
 * the various DEF, CDEF, RULE, LINE, AREA, etc statements that actually plot the charts.  The
 * rrdtool program requires that this come *last* in the argument string; we make sure that it
 * is put in it's proper place.  The $extras variable is used for other arguemnts that may not
 * fit nicely for other reasons.  Complicated requests for --color, or adding --ridgid, for example.
 * It is simply a way for the graph writer to add an arbitrary options when calling rrdtool, and to
 * forcibly override other settings, since rrdtool will use the last version of an option passed.
 * (For example, if you call 'rrdtool' with two --title statements, the second one will be used.)
 *
 * See $graphdir/sample.php for more documentation, and details on the
 * common variables passed and used.
 */

// No report requested, so use 'metric'
if (!$graph) {
    $graph = 'metric';
}


// Only in rrdtool this check is necessary
if ( $use_graphite == "no" ) {

  $graph_file = "$graphdir/$graph.php";

  if ( is_readable($graph_file) ) {
      include_once($graph_file);

      $graph_function = "graph_${graph}";
      $graph_function($rrdtool_graph);  // Pass by reference call, $rrdtool_graph modified inplace
  } else {
      /* Bad stuff happened. */
      error_log("Tried to load graph file [$graph_file], but failed.  Invalid graph, aborting.");
      exit();
  }

}

# Calculate time range.
if ($sourcetime)
   {
      $end = $sourcetime;
      # Get_context makes start negative.
      $start = $sourcetime + $start;
   }
# Fix from Phil Radden, but step is not always 15 anymore.
if ($range=="month")
   $rrdtool_graph['end'] = floor($rrdtool_graph['end'] / 672) * 672;

# Tidy up the title a bit
switch ($context) {
    case 'meta':
        $title = "$self Grid";
        break;

    case 'cluster':
        if (preg_match('/cluster/i', $clustername))
            $title  = $clustername;
        else
            $title  = "$clustername Cluster";
        break;

    case 'grid':
        if (preg_match('/grid/i', $gridname))
            $title  = $gridname;
        else
            $title  = "$gridname Grid";
        break;

    case 'host':
        if (!$summary)
            $title = null ;
        break;

    default:
        $title = $clustername;
        break;
}

if ($context != 'host') {
   $rrdtool_graph['title'] = $rrdtool_graph['title'] . " last $range";
}

if (isset($title)) {
   $rrdtool_graph['title'] = "$title " . $rrdtool_graph['title'];
}

//--------------------------------------------------------------------------------------

if ( $use_graphite == "no" ) {

  // We must have a 'series' value, or this is all for naught
  if (!array_key_exists('series', $rrdtool_graph) || !strlen($rrdtool_graph['series']) ) {
      error_log("\$series invalid for this graph request ".$_SERVER['PHP_SELF']);
      exit();
  }

}

  # Make small graphs (host list) cleaner by removing the too-big
  # legend: it is displayed above on larger cluster summary graphs.
  if ($size == "small" and ! isset($subtitle))
      $rrdtool_graph['extras'] = "-g";

  $command = RRDTOOL . " graph - $rrd_options ";

  // The order of the other arguments isn't important, except for the
  // 'extras' and 'series' values.  These two require special handling.
  // Otherwise, we just loop over them later, and tack $extras and
  // $series onto the end of the command.
  foreach (array_keys ($rrdtool_graph) as $key) {

      if (preg_match('/extras|series/', $key))
	  continue;

      $value = $rrdtool_graph[$key];

      if (preg_match('/\W/', $value)) {
	  //more than alphanumerics in value, so quote it
	  $value = "'$value'";
      }
      $command .= " --$key $value";
  }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Let's check if we are using graphite
// USING RRDTOOL
//////////////////////////////////////////////////////////////////////////////////////////////////
if ( $use_graphite == "no" ) {

  // And finish up with the two variables that need special handling.
  // See above for how these are created
  $command .= array_key_exists('extras', $rrdtool_graph) ? ' '.$rrdtool_graph['extras'].' ' : '';
  $command .= " $rrdtool_graph[series]";

} else {

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Let's check if we are using graphite
  // USING Graphite
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Check whether the link exists from Ganglia RRD tree to the graphite storage/rrd_dir
  // area
  if ( ! is_link($rrd_graphite_link) ) {
    // Does the directory exist for the cluster. If not create it
    if ( ! is_dir ($graphite_rrd_dir . "/" . $clustername) )
      mkdir ( $graphite_rrd_dir . "/" . $clustername );
    symlink($rrd_dir, $rrd_graphite_link);
  }

  // Generate host cluster string
  $host_cluster = $clustername . "." . $host;

  $height += 70;

  if ($size == "small") {
    $width += 20;
  }

//  $title = urlencode($rrdtool_graph["title"]);

  if ( isset($_GET['g'])) {

    // if it's a report increase the height for additional 30 pixels
    $height += 40;

    $report_name = sanitize($_GET['g']);

    $report_definition_file = $ganglia_dir . "/graph.d/" . $report_name . ".json";
    // Check whether report is defined in graph.d directory
    if ( is_file($report_definition_file) ) {
      $report_array = json_decode(file_get_contents($report_definition_file), TRUE);
    }

    if ( isset($report_array) ) {

      switch ( $report_array["report_type"] ) {

	case "template":
	  $target = str_replace("HOST_CLUSTER", $host_cluster, $report_array["report_arguments"]);
	  break;

	default:
	  error_log("No report_type specified in the $report_name definition.");
	  break;
      }

    }

  } else {
    // It's a simple metric graph
    $target = "target=$host_cluster.$metric_name.sum&hideLegend=true&vtitle=$vlabel&areaMode=all";
  }

  $url = $graphite_url_base . "?width=$width&height=$height&" . $target . "&title=$title&from=" . $start . "&yMin=0&bgcolor=FFFFFF&fgcolor=000000";


}

#if ($debug) {   error_log("Final rrdtool command:  $command");   }

# Did we generate a command?   Run it.
if($command) {
    /*Make sure the image is not cached*/
    header ("Expires: Mon, 26 Jul 1997 05:00:00 GMT");   // Date in the past
    header ("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT"); // always modified
    header ("Cache-Control: no-cache, must-revalidate");   // HTTP/1.1
    header ("Pragma: no-cache");                     // HTTP/1.0
    if ($debug>2) {
        header ("Content-type: text/html");
        print "<html><body>";
	if ( $use_graphite == "no" ) {
	  print htmlentities( $command );
	} else {
	  print $url;
	}
        print "</body></html>";
    } else {
        header ("Content-type: image/png");
	if ( $use_graphite == "no" ) {
	  passthru($command);
	} else {
          echo file_get_contents($url);
	}

    }
}

?>
