<?php
include_once("./conf.php");
include_once("./functions.php");
?>
<!DOCTYPE html> 
<html>
<head>
<title>Ganglia Mobile</title>
<style>
.ui-mobile .ganglia-mobile {  background: #e5e5e5 url(../images/jqm-sitebg.png) top center repeat-x; }
</style>
</head>
<body>
<?php
///////////////////////////////////////////////////////////////////////////////
// Generating mobile view
///////////////////////////////////////////////////////////////////////////////
if ( isset($_GET['view_name'])) {
?>  
  <div data-role="page" class="ganglia-mobile" id="view-home">
    <div data-role="header">
      <h1>View <?php print $_GET['view_name']; ?></h1>
      <div data-role="navbar">
	<ul>
  <?php

  $view_name = $_GET['view_name'];
  $available_views = get_available_views();
  
  // Header bar support up to 5 items. 5+ items will be shown in multiple
  // rows. Thus we'll limit to first 5 time ranges
  $my_ranges = array_keys( $time_ranges );     
  for ( $i = 0 ; $i < 5 ; $i++ ) {
     $context_ranges[] = $my_ranges[$i]; 
  }

  $range_menu = "";
  $range = $_GET['r'];

  foreach ($context_ranges as $v) {
     $url=rawurlencode($v);
     if ($v == $range)
       $checked = "class=\"ui-btn-active\"";
     else
       $checked = "";

     $range_menu .= "<li><a href='mobile-helper.php?view_name=" . $_GET['view_name'] . "&r=" . $v . "&cs=&ce='>$v</a></li>";
  }
    print $range_menu;
  ?>
	  </ul>
      </div><!-- /navbar -->
    </div><!-- /header -->
  
    <div data-role="content">	
  <?php

    // Let's find the view definition
    foreach ( $available_views as $view_id => $view ) {
  
     if ( $view['view_name'] == $view_name ) {
  
      $view_elements = get_view_graph_elements($view);

      $range_args = "";
      if ( isset($_GET['r']) && $_GET['r'] != "" ) 
	    $range_args .= "&r=" . $_GET['r'];
      if ( isset($_GET['cs']) && isset($_GET['ce']) ) 
	    $range_args .= "&cs=" . $_GET['cs'] . "&ce=" . $_GET['ce'];

      foreach ( $view_elements as $id => $element ) {
	print "
	<A HREF=\"./graph_all_periods.php?mobile=1&" . $element['graph_args'] ."&z=mobile\">
	<IMG ALT=\"" . $element['hostname'] . " - " . $element['name'] . "\" BORDER=0 SRC=\"./graph.php?" . $element['graph_args'] . "&z=mobile" . $range_args .  "\"></A>";
      }
  
     }  // end of if ( $view['view_name'] == $view_name
    } // end of foreach ( $views as $view_id 

  
    print "</div><!-- /content -->
    </div> <!-- /page -->";
} // end of if ( isset($_GET['view_name']))
///////////////////////////////////////////////////////////////////////////////
// Generate host view
///////////////////////////////////////////////////////////////////////////////
if ( isset($_GET['show_host_metrics'])) {
  $hostname = $_GET['h'];
  $clustername = $_GET['c'];
?>  
  <div data-role="page" class="ganglia-mobile" id="viewhost-<?php print $hostname; ?>">
    <div data-role="header">
      <h2>Hostname <?php print $hostname; ?></h2>
        <div data-role="navbar">
	<ul>
  <?php
	// Header bar support up to 5 items. 5+ items will be shown in multiple
	// rows. Thus we'll limit to first 5 time ranges
	$my_ranges = array_keys( $time_ranges );     
	for ( $i = 0 ; $i < 5 ; $i++ ) {
	   $context_ranges[] = $my_ranges[$i]; 
	}
      
	$range_menu = "";
	$range = $_GET['r'];
      
	foreach ($context_ranges as $v) {
	   $url=rawurlencode($v);
	   if ($v == $range)
	     $checked = "class=\"ui-btn-active\"";
	   else
	     $checked = "";
      
	   $range_menu .= "<li><a href='mobile-helper.php?show_host_metrics=1&h=" . $hostname . "&c=" . $clustername . "&r=" . $v . "&cs=&ce='>$v</a></li>";
	}
	  print $range_menu;
    ?>
	</ul>
      </div><!-- /navbar -->
    </div><!-- /header -->
  
    <div data-role="content">
<?php
    $graph_args = "h=$hostname&c=$clustername&r=$range";
    
    ####################################################################################
    # Let's find out what optional reports are included
    # First we find out what the default (site-wide) reports are then look
    # for host specific included or excluded reports
    ####################################################################################
    $default_reports = array("included_reports" => array(), "excluded_reports" => array());
    if ( is_file($GLOBALS['conf_dir'] . "/default.json") ) {
      $default_reports = array_merge($default_reports,json_decode(file_get_contents($GLOBALS['conf_dir'] . "/default.json"), TRUE));
    }
    
    $host_file = $GLOBALS['conf_dir'] . "/host_" . $hostname . ".json";
    $override_reports = array("included_reports" => array(), "excluded_reports" => array());
    if ( is_file($host_file) ) {
      $override_reports = array_merge($override_reports, json_decode(file_get_contents($host_file), TRUE));
    }
    
    # Merge arrays
    $reports["included_reports"] = array_merge( $default_reports["included_reports"] , $override_reports["included_reports"]);
    $reports["excluded_reports"] = array_merge($default_reports["excluded_reports"] , $override_reports["excluded_reports"]);
    
    # Remove duplicates
    $reports["included_reports"] = array_unique($reports["included_reports"]);
    $reports["excluded_reports"] = array_unique($reports["excluded_reports"]);
    
    foreach ( $reports["included_reports"] as $index => $report_name ) {
    
      if ( ! in_array( $report_name, $reports["excluded_reports"] ) ) {
	print "
	<A HREF=\"./graph_all_periods.php?mobile=1&$graph_args&amp;g=" . $report_name . "&amp;z=large\">
	<IMG BORDER=0 ALT=\"$clustername\" SRC=\"./graph.php?$graph_args&amp;g=" . $report_name ."&amp;z=medium\"></A>";
      }
    }
    ?>
    <p><font color=red>Metric graphs not implemented yet.</font>
    </div><!-- /content -->
  </div><!-- /page -->
<?php
}
?>  

</body>
</html>
