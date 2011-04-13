<?php
/* $Id$ */
include_once "./eval_conf.php";
include_once "./get_context.php";
include_once "./functions.php";

$ganglia_dir = dirname(__FILE__);

# RFM - Added all the isset() tests to eliminate "undefined index"
# messages in ssl_error_log.

# Graph specific variables
# ATD - No need for escapeshellcmd or rawurldecode on $size or $graph.  Not used directly in rrdtool calls.
$size = isset($_GET["z"]) && in_array( $_GET[ 'z' ], $conf['graph_sizes_keys'] )
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

$user['json_output'] = isset($_GET["json"]) ? 1 : NULL; 
$user['csv_output'] = isset($_GET["csv"]) ? 1 : NULL; 
$user['flot_output'] = isset($_GET["flot"]) ? 1 : NULL; 


// Get hostname
$raw_host = isset($_GET["h"])  ?  sanitize ( $_GET["h"]  )   : "__SummaryInfo__";  

// For graphite purposes we need to replace all dots with underscore. dot  is
// separates subtrees in graphite
$host = str_replace(".","_", $raw_host);

# Assumes we have a $start variable (set in get_context.php).
# $conf['graph_sizes'] and $conf['graph_sizes_keys'] defined in conf.php.  Add custom sizes there.
$size = in_array( $size, $conf['graph_sizes_keys'] ) ? $size : 'default';

if ( isset($_GET['height'] ) ) 
  $height = $_GET['height'];
else 
  $height  = $conf['graph_sizes'][ $size ][ 'height' ];

if ( isset($_GET['width'] ) ) 
  $width =  $_GET['width'];
else
  $width = $conf['graph_sizes'][ $size ][ 'width' ];

#$height  = $conf['graph_sizes'][ $size ][ 'height' ];
#$width   = $conf['graph_sizes'][ $size ][ 'width' ];
$fudge_0 = $conf['graph_sizes'][ $size ][ 'fudge_0' ];
$fudge_1 = $conf['graph_sizes'][ $size ][ 'fudge_1' ];
$fudge_2 = $conf['graph_sizes'][ $size ][ 'fudge_2' ];

///////////////////////////////////////////////////////////////////////////
// Set some variables depending on the context. Context is set in
// get_context.php
///////////////////////////////////////////////////////////////////////////
switch ($context)
{
    case "meta":
      $rrd_dir = $conf['rrds'] . "/__SummaryInfo__";
      $rrd_graphite_link = $conf['graphite_rrd_dir'] . "/__SummaryInfo__";
      $title = "$self Grid";
      break;
    case "grid":
      $rrd_dir = $conf['rrds'] . "/$grid/__SummaryInfo__";
      $rrd_graphite_link = $conf['graphite_rrd_dir'] . "/$grid/__SummaryInfo__";
      if (preg_match('/grid/i', $gridname))
          $title  = $gridname;
      else
          $title  = "$gridname Grid";
      break;
    case "cluster":
      $rrd_dir = $conf['rrds'] . "/$clustername/__SummaryInfo__";
      $rrd_graphite_link = $conf['graphite_rrd_dir'] . "/$clustername/__SummaryInfo__";
      if (preg_match('/cluster/i', $clustername))
          $title  = $clustername;
      else
          $title  = "$clustername Cluster";
      break;
    case "host":
      $rrd_dir = $conf['rrds'] . "/$clustername/$raw_host";
      $rrd_graphite_link = $conf['graphite_rrd_dir'] . "/" . $clustername . "/" . $host;
      $title = "";
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
    $conf['strip_domainname'] = true;
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
 * See ${conf['graphdir']}/sample.php for more documentation, and details on the
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

// Are we generating aggregate graphs
if ( isset( $_GET["aggregate"] ) && $_GET['aggregate'] == 1 ) {
    
    // Set start time
    $start = time() + $start;

    // If graph type is not specified default to line graph
    if ( isset($_GET["gtype"]) && in_array($_GET["gtype"], array("stack","line") )  ) 
        $graph_type = $_GET["gtype"];
    else
        $graph_type = "line";
    
    // If line width not specified default to 2
    if ( isset($_GET["lw"]) && in_array($_GET["lw"], array("1","2", "3") )  ) 
        $line_width = $_GET["lw"];
    else
        $line_width = "2";
    
    // Set up 
    $graph_config["report_name"] = $metric_name;
    $graph_config["report_type"] = "standard";
    $graph_config["title"] = $metric_name . " last " . $range;

    // Colors to use
    $colors = array("128936","FF8000","158499","CC3300","996699","FFAB11","3366CC","01476F");
    $color_count = sizeof($colors);

    // Load the host cache
    require_once('./cache.php');
    
    $counter = 0;

    // Find matching hosts    
    foreach ( $_GET['hreg'] as $key => $query ) {
      foreach ( $index_array['hosts'] as $key => $host_name ) {
        if ( preg_match("/$query/i", $host_name ) ) {
          // We can have same hostname in multiple clusters
          $matches[] = $host_name . "|" . $index_array['cluster'][$host_name]; 
        }
      }
    } 

    if ( isset($matches)) {

        $matches_unique = array_unique($matches);
    
        // Create graph_config series from matched hosts
        foreach ( $matches_unique as $key => $host_cluster ) {
          // We need to cycle the available colors
          $color_index = $counter % $color_count;
          
          $out = explode("|", $host_cluster);
          
          $host_name = $out[0];
          $cluster_name = $out[1];
          
          $graph_config['series'][] = array ( "hostname" => $host_name , "clustername" => $cluster_name,
             "metric" => $metric_name,  "color" => $colors[$color_index], "label" => $host_name, "line_width" => $line_width, "type" => $graph_type);
             $counter++;
     
        }
    
    }
    #print "<PRE>"; print_r($graph_config); exit(1);
 
}

//////////////////////////////////////////////////////////////////////////////
// Check what graph engine we are using
//////////////////////////////////////////////////////////////////////////////
switch ( $conf['graph_engine'] ) {
  case "rrdtool":
    
    if ( ! isset($graph_config) ) {
        $php_report_file = $conf['graphdir'] . "/" . $graph . ".php";
        $json_report_file = $conf['graphdir'] . "/" . $graph . ".json";
        if( is_file( $php_report_file ) ) {
          include_once $php_report_file;
          $graph_function = "graph_${graph}";
          $graph_function( $rrdtool_graph );  // Pass by reference call, $rrdtool_graph modified inplace
        } else if ( is_file( $json_report_file ) ) {
          $graph_config = json_decode( file_get_contents( $json_report_file ), TRUE );
          
          # We need to add hostname and clustername if it's not specified
          foreach ( $graph_config['series'] as $index => $item ) {
            if ( ! isset($graph_config['series'][$index]['hostname'])) {
              $graph_config['series'][$index]['hostname'] = $raw_host;
              $graph_config['series'][$index]['clustername'] = $clustername;
            }
          }
          
          build_rrdtool_args_from_json ( $rrdtool_graph, $graph_config );
        }
    
    } else {
        
        build_rrdtool_args_from_json ( $rrdtool_graph, $graph_config );

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

    # add slope-mode if rrdtool_slope_mode is set
    if (isset($conf['rrdtool_slope_mode']) && $conf['rrdtool_slope_mode'] == True)
        $rrdtool_graph['slope-mode'] = '';
  
    $command = $conf['rrdtool'] . " graph - $rrd_options ";
  
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
      if ( ! is_dir ($conf['graphite_rrd_dir'] . "/" . str_replace(" ", "_", $clustername)) )
        mkdir ( $conf['graphite_rrd_dir'] . "/" . str_replace(" ", "_", $clustername ));
      symlink($rrd_dir, str_replace(" ", "_", $rrd_graphite_link));
    }
  
    // Generate host cluster string
    if ( isset($clustername) ) {
      $host_cluster = str_replace(" ", "_", $clustername) . "." . $host;
    } else {
      $host_cluster = $host;
    }
  
    $height += 70;
  
    if ($size == "small") {
      $width += 20;
    }
  
  //  $title = urlencode($rrdtool_graph["title"]);
  
    // If graph_config is already set we can use it immediately
    if ( isset($graph_config) ) {

      $target = build_graphite_series( $graph_config, "" );

    } else {

      if ( isset($_GET['g'])) {
	// if it's a report increase the height for additional 30 pixels
	$height += 40;
    
	$report_name = sanitize($_GET['g']);
    
	$report_definition_file = $conf['ganglia_dir'] . "/graph.d/" . $report_name . ".json";
	// Check whether report is defined in graph.d directory
	if ( is_file($report_definition_file) ) {
	  $graph_config = json_decode(file_get_contents($report_definition_file), TRUE);
	} else {
	  error_log("There is JSON config file specifying $report_name.");
	  exit(1);
	}
    
	if ( isset($graph_config) ) {
	  switch ( $graph_config["report_type"] ) {
	    case "template":
	      $target = str_replace("HOST_CLUSTER", $host_cluster, $graph_config["graphite"]);
	      break;
    
	    case "standard":
	      $target = build_graphite_series( $graph_config, $host_cluster );
	      break;
    
	    default:
	      error_log("No valid report_type specified in the $report_name definition.");
	      break;
	  }
    
	  $title = $graph_config['title'];
	} else {
	  error_log("Configuration file to $report_name exists however it doesn't appear it's a valid JSON file");
	  exit(1);
	}
      } else {
	// It's a simple metric graph
	$target = "target=$host_cluster.$metric_name.sum&hideLegend=true&vtitle=$vlabel&areaMode=all";
	$title = " ";
      }

    } // end of if ( ! isset($graph_config) ) {
    
    $graphite_url = $conf['graphite_url_base'] . "?width=$width&height=$height&" . $target . "&from=" . $start . "&yMin=0&bgcolor=FFFFFF&fgcolor=000000&title=" . urlencode($title . " last " . $range);
    break;

} // end of switch ( $conf['graph_engine'])


// Output to JSON
if ( $user['json_output'] || $user['csv_output'] || $user['flot_output'] ) {

  $rrdtool_graph_args = "";

  // First find RRDtool DEFs by parsing $rrdtool_graph['series']
  preg_match_all("| DEF:(.*):AVERAGE|U", " " . $rrdtool_graph['series'], $matches);

  foreach ( $matches[0] as $key => $value ) {
    if ( preg_match("/(DEF:\')(.*)(\'=\')(.*)\/(.*)\/(.*)\/(.*)(\.rrd)/", $value, $out ) ) {
	$ds_name = $out[2];
	$cluster_name = $out[5];
	$host_name = $out[6];
	$metric_name = $out[7];
	$output_array[] = array( "ds_name" => $ds_name, "cluster_name" => $out[5], "host_name" => $out[6], "metric_name" => $out[7] );
	$rrdtool_graph_args .= $value . " " . "XPORT:" . $ds_name . ":" . $metric_name . " ";
    }
  }

  // This command will export values for the specified format in XML
  $command = $conf['rrdtool'] . " xport --start " . $rrdtool_graph['start'] . " --end " .  $rrdtool_graph['end'] . " " . $rrdtool_graph_args;

  // Read in the XML
  $fp = popen($command,"r"); 
  $string = "";
  while (!feof($fp)) { 
    $buffer = fgets($fp, 4096);
    $string .= $buffer;
  }
  // Parse it
  $xml = simplexml_load_string($string);

  # If there are multiple metrics columns will be > 1
  $num_of_metrics = $xml->meta->columns;

  // 
  $metric_values = array();
  // Build the metric_values array

  foreach ( $xml->data->row as $key => $objects ) {
    $values = get_object_vars($objects);
    // If $values["v"] is an array we have multiple data sources/metrics and we 
    // need to iterate over those
    if ( is_array($values["v"]) ) {
      foreach ( $values["v"] as $key => $value ) {
        $output_array[$key]["metrics"][] = array( "timestamp" => intval($values['t']), "value" => floatval($value));
      }
    } else {
      $output_array[0]["metrics"][] = array( "timestamp" => intval($values['t']), "value" => floatval($values['v']));
    }

  }

  // If JSON output request simple encode the array as JSON
  if ( $user['json_output'] ) {

    header("Content-type: application/json");
    header("Content-Disposition: inline; filename=\"ganglia-metrics.json\"");
    print json_encode($output_array);

  }

  // If Flot output massage the data JSON
  if ( $user['flot_output'] ) {

    foreach ( $output_array as $key => $metric_array ) {
      foreach ( $metric_array['metrics'] as $key => $values ) {
	$data_array[] = array ( $values['timestamp'] * 1000,  $values['value']);  
      }

      $flot_array[] = array( 'label' =>  strip_domainname($metric_array['host_name']) . " " . $metric_array['metric_name'], 
	  'data' => $data_array);

      unset($data_array);

    }

    header("Content-type: application/json");
    print json_encode($flot_array);

  }

  if ( $user['csv_output'] ) {

    header("Content-Type: application/csv");
    header("Content-Disposition: inline; filename=\"ganglia-metrics.csv\"");

    print "Timestamp";

    // Print out headers
    for ( $i = 0 ; $i < sizeof($output_array) ; $i++ ) {
      print "," . $output_array[$i]["metric_name"];
    }

    print "\n";

    foreach ( $output_array[0]["metrics"] as $key => $row ) {
      print date("c", $row["timestamp"]);
      for ( $j = 0 ; $j < $num_of_metrics ; $j++ ) {
	print "," .$output_array[$j]["metrics"][$key]["value"];
      }
      print "\n";
    }

  }

  exit(1);
}


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
        
        switch ( $conf['graph_engine'] ) {  
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
        switch ( $conf['graph_engine'] ) {  
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
