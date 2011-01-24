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

# If graph arg is not specified default to metric
$graph      = isset($_GET["g"])  ?  sanitize ( $_GET["g"] )   : "metric";
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
// 
$command    = '';
$graphite_url = '';

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

///////////////////////////////////////////////////////////////////////////
// Set some variables depending on the context. Context is set in
// get_context.php
///////////////////////////////////////////////////////////////////////////
switch ($context)
{
    case "meta":
      $rrd_dir = "$rrds/__SummaryInfo__";
      $rrd_graphite_link = "$graphite_rrd_dir/__SummaryInfo__";
      $title = "$self Grid";
      break;
    case "grid":
      $rrd_dir = "$rrds/$grid/__SummaryInfo__";
      $rrd_graphite_link = "$graphite_rrd_dir/$grid/__SummaryInfo__";
      if (preg_match('/grid/i', $gridname))
          $title  = $gridname;
      else
          $title  = "$gridname Grid";
      break;
    case "cluster":
      $rrd_dir = "$rrds/$clustername/__SummaryInfo__";
      $rrd_graphite_link = "$graphite_rrd_dir/$clustername/__SummaryInfo__";
      if (preg_match('/cluster/i', $clustername))
          $title  = $clustername;
      else
          $title  = "$clustername Cluster";
      break;
    case "host":
      $rrd_dir = "$rrds/$clustername/$raw_host";
      $rrd_graphite_link = $graphite_rrd_dir . "/" . $clustername . "/" . $host;
      if (!$summary)
        $title = $metric_name ;
      break;
    default:
      $title = $clustername;
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


// Calculate time range.
if ($sourcetime)
   {
      $end = $sourcetime;
      # Get_context makes start negative.
      $start = $sourcetime + $start;
   }
// Fix from Phil Radden, but step is not always 15 anymore.
if ($range == "month")
   $rrdtool_graph['end'] = floor($rrdtool_graph['end'] / 672) * 672;

if ( isset( $rrdtool_graph['title'])) 
   $rrdtool_graph['title'] = $rrdtool_graph['title'] . " last $range";
else
   $rrdtool_graph['title'] = $title . " last $range";

if (isset($title)) {
   $rrdtool_graph['title'] = $title . " last $range";
}

//////////////////////////////////////////////////////////////////////////////
// Check what graph engine we are using
//////////////////////////////////////////////////////////////////////////////
switch ( $GLOBALS['graph_engine'] ) {

  case "rrdtool":  
  
    $php_report_file = $graphdir . "/" . $graph . ".php";
    $json_report_file = $graphdir . "/" . $graph . ".json";
    if( is_file( $php_report_file ) ) {
      include_once $php_report_file;
      $graph_function = "graph_${graph}";
      $graph_function( $rrdtool_graph );  // Pass by reference call, $rrdtool_graph modified inplace
    } else if ( is_file( $json_report_file ) ) {
      $config = json_decode( file_get_contents( $json_report_file ), TRUE );
      
      # We need to add hostname and clustername if it's not specified
      foreach ( $config['series'] as $index => $item ) {
        if ( ! isset($config['series'][$index]['hostname'])) {
          $config['series'][$index]['hostname'] = $raw_host;
          $config['series'][$index]['clustername'] = $clustername;
        }
      }
      
      build_rrdtool_args_from_json ( $rrdtool_graph, $config );
    }
  
    // We must have a 'series' value, or this is all for naught
    if (!array_key_exists('series', $rrdtool_graph) || !strlen($rrdtool_graph['series']) ) {
        error_log("\$series invalid for this graph request ".$_SERVER['PHP_SELF']);
        exit();
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
  
    // And finish up with the two variables that need special handling.
    // See above for how these are created
    $command .= array_key_exists('extras', $rrdtool_graph) ? ' '.$rrdtool_graph['extras'].' ' : '';
    $command .= " $rrdtool_graph[series]";

    break;


  //////////////////////////////////////////////////////////////////////////////////////////////////
  // USING Graphite
  //////////////////////////////////////////////////////////////////////////////////////////////////
  case "graphite":  

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
        $config = json_decode(file_get_contents($report_definition_file), TRUE);
      } else {
        error_log("There is JSON config file specifying $report_name.");
        exit(1);
      }
  
      if ( isset($config) ) {
  
        switch ( $config["report_type"] ) {
  
          case "template":
            $target = str_replace("HOST_CLUSTER", $host_cluster, $config["graphite"]);
            break;
  
          case "standard":
            $target = build_graphite_series( $config, $host_cluster );
            break;
  
          default:
            error_log("No valid report_type specified in the $report_name definition.");
            break;
        }
  
        $title = $config['title'];
  
      } else {
        error_log("Configuration file to $report_name exists however it doesn't appear it's a valid JSON file");
        exit(1);
  
      }
  
    } else {
      // It's a simple metric graph
      $target = "target=$host_cluster.$metric_name.sum&hideLegend=true&vtitle=$vlabel&areaMode=all";
      $title = " ";
      
    }
  
  
    $graphite_url = $graphite_url_base . "?width=$width&height=$height&" . $target . "&from=" . $start . "&yMin=0&bgcolor=FFFFFF&fgcolor=000000&title=" . urlencode($title . " last " . $range);

    break;

} // end of switch ( $GLOBALS['graph_engine'])


if ($debug) {
  error_log("Final rrdtool command:  $command");
}

# Did we generate a command?   Run it.
if($command || $graphite_url) {
    /*Make sure the image is not cached*/
    header ("Expires: Mon, 26 Jul 1997 05:00:00 GMT");   // Date in the past
    header ("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT"); // always modified
    header ("Cache-Control: no-cache, must-revalidate");   // HTTP/1.1
    header ("Pragma: no-cache");                     // HTTP/1.0
    if ($debug>2) {
        header ("Content-type: text/html");
        print "<html><body>";
        
        switch ( $GLOBALS['graph_engine'] ) {  
          case "rrdtool":
            print htmlentities( $command );
            break;
          case "graphite":
            print $graphite_url;
            break;
          
        }        
        print "</body></html>";
    } else {
      
        header ("Content-type: image/png");
        switch ( $GLOBALS['graph_engine'] ) {  
          case "rrdtool":
            passthru($command);
            break;
          case "graphite":
            echo file_get_contents($graphite_url);
            break;
          
        }        

    }
}

?>
