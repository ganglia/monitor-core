<?php
include_once "./eval_conf.php";
include_once("./functions.php");
include_once "./get_context.php";
include_once "./ganglia.php";
include_once "./get_ganglia.php";
?>
<?php
///////////////////////////////////////////////////////////////////////////////
// Generating mobile view
///////////////////////////////////////////////////////////////////////////////
if ( isset($_GET['view_name'])) {
?>  
  <div data-role="page" class="ganglia-mobile" id="view-home">
    <div data-role="header">
      <a href="#" class="ui-btn-left" data-icon="arrow-l" onclick="history.back(); return false">Back</a>
      <h1>View <?php print $_GET['view_name']; ?></h1>
      <a href="#mobile-home">Home</a>
      <div data-role="navbar">
	<ul>
  <?php

  $view_name = $_GET['view_name'];
  $available_views = get_available_views();
  
  // Header bar support up to 5 items. 5+ items will be shown in multiple
  // rows. Thus we'll limit to first 5 time ranges
  $my_ranges = array_keys( $conf['time_ranges'] );     
  for ( $i = 0 ; $i < 5 ; $i++ ) {
     $context_ranges[] = $my_ranges[$i]; 
  }

  $range_menu = "";
  $range = $_GET['r'];

  foreach ($context_ranges as $v) {
     $url=rawurlencode($v);
     if ($v == $range) {
      $checked = "class=\"ui-btn-active\"";
      $range_menu .= "<li><a $checked href='#' onclick='return false;'>$v</a></li>";
    } else {
      $range_menu .= "<li><a href='mobile_helper.php?view_name=" . $_GET['view_name'] . "&r=" . $v . "&cs=&ce='>$v</a></li>";
    }

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

      if ( count($view_elements) != 0 ) {
	foreach ( $view_elements as $id => $element ) {
	  print "
	  <A HREF=\"./graph_all_periods.php?mobile=1&" . $element['graph_args'] ."&z=mobile\">
	  <IMG ALT=\"" . $element['hostname'] . " - " . $element['name'] . "\" BORDER=0 SRC=\"./graph.php?" . $element['graph_args'] . "&z=mobile" . $range_args .  "\"></A>";
	}
      } else {
	print "No graphs defined for this view. Please add some";
      }
	
	
  
     }  // end of if ( $view['view_name'] == $view_name
    } // end of foreach ( $views as $view_id 

  
    print "</div><!-- /content -->
    </div> <!-- /page -->";
} // end of if ( isset($_GET['view_name']))
///////////////////////////////////////////////////////////////////////////////
// Generate cluster summary view
///////////////////////////////////////////////////////////////////////////////
if ( isset($_GET['show_cluster_metrics'])) {
  $clustername = $_GET['c'];
?>  
  <div data-role="page" class="ganglia-mobile" id="viewhost-<?php print $hostname; ?>">
    <div data-role="header" data-position="fixed">
      <a href="#" class="ui-btn-left" data-icon="arrow-l" onclick="history.back(); return false">Back</a>
      <h3>Cluster <?php print $clustername; ?></h3>
      <a href="#mobile-home">Home</a>
        <div data-role="navbar">
	<ul>
  <?php
	// Header bar support up to 5 items. 5+ items will be shown in multiple
	// rows. Thus we'll limit to first 5 time ranges
	$my_ranges = array_keys( $conf['time_ranges'] );     
	for ( $i = 0 ; $i < 5 ; $i++ ) {
	   $context_ranges[] = $my_ranges[$i]; 
	}
      
	$range_menu = "";
	$range = $_GET['r'];
      
	foreach ($context_ranges as $v) {
	   $url=rawurlencode($v);
	   if ($v == $range) {
	     $checked = "class=\"ui-btn-active\"";
      	     $range_menu .= "<li><a $checked href='#'>$v</a></li>";
	  } else {
      	     $range_menu .= "<li><a href='mobile_helper.php?show_cluster_metrics=1&c=" . $clustername . "&r=" . $v . "&cs=&ce='>$v</a></li>";
	  }
	}
	  print $range_menu;
    ?>
	</ul>
      </div><!-- /navbar -->
    </div><!-- /header -->
  
    <div data-role="content">
<?php
    $graph_args = "c=$clustername&r=$range";
    
    ///////////////////////////////////////////////////////////////////////////
    // Let's find out what optional reports are included
    // First we find out what the default (site-wide) reports are then look
    // for host specific included or excluded reports
    ///////////////////////////////////////////////////////////////////////////
    $default_reports = array("included_reports" => array(), "excluded_reports" => array());
    if ( is_file($conf['conf_dir'] . "/default.json") ) {
      $default_reports = array_merge($default_reports,json_decode(file_get_contents($conf['conf_dir'] . "/default.json"), TRUE));
    }
    
    $cluster_file = $conf['conf_dir'] . "/cluster_" . $clustername . ".json";
    $override_reports = array("included_reports" => array(), "excluded_reports" => array());
    if ( is_file($cluster_file) ) {
      $override_reports = array_merge($override_reports, json_decode(file_get_contents($cluster_file), TRUE));
    }
    
    # Merge arrays
    $reports["included_reports"] = array_merge( $default_reports["included_reports"] , $override_reports["included_reports"]);
    $reports["excluded_reports"] = array_merge($default_reports["excluded_reports"] , $override_reports["excluded_reports"]);
    
    # Remove duplicates
    $reports["included_reports"] = array_unique($reports["included_reports"]);
    $reports["excluded_reports"] = array_unique($reports["excluded_reports"]);
    
    foreach ( $reports["included_reports"] as $index => $report_name ) {
      if ( ! in_array( $report_name, $reports["excluded_reports"] ) ) {
	print "<a name=metric_" . $report_name . ">
	<A HREF=\"./graph_all_periods.php?mobile=1&$graph_args&amp;g=" . $report_name . "&amp;z=mobile&amp;c=$clustername\">
	<IMG BORDER=0 ALT=\"$clustername\" SRC=\"./graph.php?$graph_args&amp;g=" . $report_name ."&amp;z=mobile&amp;c=$clustername\"></A>
	";
      }

    }

?>
      </div><!-- /content -->
    </div> <!-- /page -->";
<?php
}
///////////////////////////////////////////////////////////////////////////////
// Generate host view
///////////////////////////////////////////////////////////////////////////////
if ( isset($_GET['show_host_metrics'])) {
  $hostname = $_GET['h'];
  $clustername = $_GET['c'];
?>
  <div data-role="page" class="ganglia-mobile" id="viewhost-<?php print $hostname; ?>">
    <div data-role="header" data-position="fixed">
      <a href="#" class="ui-btn-left" data-icon="arrow-l" onclick="history.back(); return false">Back</a>
      <h3>Host <?php print $hostname; ?></h3>
      <a href="#mobile-home">Home</a>
        <div data-role="navbar">
	<ul>
  <?php
	// Header bar support up to 5 items. 5+ items will be shown in multiple
	// rows. Thus we'll limit to first 5 time ranges
	$my_ranges = array_keys( $conf['time_ranges'] );     
	for ( $i = 0 ; $i < 5 ; $i++ ) {
	   $context_ranges[] = $my_ranges[$i]; 
	}
      
	$range_menu = "";
	$range = $_GET['r'];
      
	foreach ($context_ranges as $v) {
	   $url=rawurlencode($v);
	   if ($v == $range) {
	     $checked = "class=\"ui-btn-active\"";
      	     $range_menu .= "<li><a $checked href='#'>$v</a></li>";
	  } else {
      	     $range_menu .= "<li><a href='mobile_helper.php?show_host_metrics=1&h=" . $hostname . "&c=" . $clustername . "&r=" . $v . "&cs=&ce='>$v</a></li>";
	  }
	}
	  print $range_menu;
    ?>
	</ul>
      </div><!-- /navbar -->
    </div><!-- /header -->
  
    <div data-role="content">
<?php
    $graph_args = "h=$hostname&c=$clustername&r=$range";
    
    ///////////////////////////////////////////////////////////////////////////
    // Let's find out what optional reports are included
    // First we find out what the default (site-wide) reports are then look
    // for host specific included or excluded reports
    ///////////////////////////////////////////////////////////////////////////
    $default_reports = array("included_reports" => array(), "excluded_reports" => array());
    if ( is_file($conf['conf_dir'] . "/default.json") ) {
      $default_reports = array_merge($default_reports,json_decode(file_get_contents($conf['conf_dir'] . "/default.json"), TRUE));
    }
    
    $host_file = $conf['conf_dir'] . "/host_" . $hostname . ".json";
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
	<IMG BORDER=0 ALT=\"$clustername\" SRC=\"./graph.php?$graph_args&amp;g=" . $report_name ."&amp;z=mobile\"></A>";
      }
    }
    ?>  
<?php

$g_metrics_group = array();
$groups = array();

$size = "mobile";

foreach ($metrics as $metric_name => $metric_attributes) {

  if ($metric_attributes['TYPE'] == "string" or $metric_attributes['TYPE']=="timestamp" or
      (isset($always_timestamp[$metric_name]) and $always_timestamp[$metric_name])) {
	$s_metrics[$metric_name] = $v;
  } elseif ($metric_attributes['SLOPE'] == "zero" or (isset($always_constant[$metric_name]) and $always_constant[$metric_name])) {
	$c_metrics[$metric_name] = $v;
  } else if (isset($reports[$metric_name]) and $reports[$metric])
    continue;
  else {
    $metric_graphargs = "c=$clustername&amp;h=$hostname&amp;v=$metric_attributes[VAL]"
      ."&amp;m=$metric_name&amp;r=$range&amp;z=$size&amp;jr=$jobrange"
      ."&amp;js=$jobstart&amp;st=$cluster[LOCALTIME]";
    if ($cs)
       $metric_graphargs .= "&amp;cs=" . rawurlencode($cs);
    if ($ce)
       $metric_graphargs .= "&amp;ce=" . rawurlencode($ce);
    # Adding units to graph 2003 by Jason Smith <smithj4@bnl.gov>.
    if ($metric_attributes['UNITS']) {
       $encodeUnits = rawurlencode($metric_attributes['UNITS']);
       $metric_graphargs .= "&amp;vl=$encodeUnits";
    }
    if (isset($metric_attributes['TITLE'])) {
       $title = $metric_attributes['TITLE'];
       $metric_graphargs .= "&amp;ti=$title";
    }
    $g_metrics[$metric_name]['graph'] = $graph_args . "&" . $metric_graphargs;
    $g_metrics[$metric_name]['description'] = isset($metric_attributes['DESC']) ? $metric_attributes['DESC'] : '';

    if ( !isset($metrics[$metric_name]['GROUP']) )
      $group_name = "no group";
    else
      $group_name = $metrics[$metric_name]['GROUP'][0];

    // Make an array of groups
    if ( ! in_array($group_name, $groups) ) {
      $groups[] = $group_name;
    }
    
    $g_metrics_group[$group_name][] = $metric_name;
 }
}

ksort($g_metrics_group);
foreach ( $g_metrics_group as $metric_group_name => $metric_group_members ) {
?>
      <div data-role="collapsible" data-collapsed="true">
	<h3><?php print $metric_group_name . " (" . sizeof($metric_group_members) . ")"; ?></h3>
<?php
      foreach ( $metric_group_members as $index => $metric_name ) {
	print "
	<A HREF=\"./graph_all_periods.php?mobile=1&" . $g_metrics[$metric_name]['graph'] .  "\">
	<IMG BORDER=0 ALT=\"$clustername\" SRC=\"./graph.php?" . $g_metrics[$metric_name]['graph'] . "\"></A>";
      }
?>
      </div> <!-- /collapsible -->
<?php
} // end of foreach ( $g_metrics_group
?>
    </div><!-- /content -->
  </div><!-- /page -->
<?php
}
?>  
