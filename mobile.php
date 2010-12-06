<?php

include_once("./conf.php");
include_once("./functions.php");

// Load the metric caching code
require_once('./cache.php');

/////////////////////////////////////////////////////////////////////////////
// With Mobile view we are gonna utilize the capability of putting in
// multiple pages in the same payload so that we avoid HTTP round trips
/////////////////////////////////////////////////////////////////////////////
?>
<!DOCTYPE html> 
<html> 
<head> 
<title>Ganglia Mobile</title> 
<link rel="stylesheet" href="css/jquery.mobile-1.0a2.min.css" />
<script src="js/jquery-1.4.4.min.js"></script>
<script src="js/jquery.mobile-1.0a2.min.js"></script>
<style>
.ui-mobile .ganglia-mobile {  background: #e5e5e5 url(../images/jqm-sitebg.png) top center repeat-x; }
.ui-mobile #mobile-homeheader { padding: 55px 25px 0; text-align: center }
.ui-mobile #mobile-homeheader h1 { margin: 0 0 10px; }
.ui-mobile #mobile-homeheader p { margin: 0; }
.ui-mobile #mobile-version { text-indent: -99999px; background: url(../images/version.png) top right no-repeat; width: 119px; height: 122px; overflow: hidden; position: absolute; top: 0; right: 0; }
.ui-mobile .mobile-themeswitcher { clear: both; margin: 20px 0 0; }
h2 { margin-top:1.5em; }
p code { font-size:1.2em; font-weight:bold; } 
dt { font-weight: bold; margin: 2em 0 .5em; }
dt code, dd code { font-size:1.3em; line-height:150%; }
</style>
</head> 
<body> 
<?php
  //  Build cluster array. So we know if there is more than 1
  foreach ( $index_array['cluster'] as $hostname => $clustername ) {
    $cluster_array[$clustername][] = $hostname;
  }
  $cluster_names = array_keys($cluster_array);
  
  $available_views = get_available_views();

?>
<div data-role="page" class="ganglia-mobile" id="mobile-home">
  <div data-role="header">
    <h1>Ganglia Mobile</h1>
  </div>
  <div data-role="content">
    Please select a category:<p>
    <ul data-role="listview" data-theme="g">
      <li><a href="#views">Views</a><span class="ui-li-count"><?php print sizeof($available_views); ?></span></li>
      <?php
	if ( sizeof($cluster_names) == 1) {
           print '<li><a href="#cluster-' . str_replace(" ", "_", $clustername) . '">Clusters</a><span class="ui-li-count">1</span></li>';
        } else {
      ?>
      <li><a href="#clusters">Clusters</a><span class="ui-li-count"><?php print sizeof($cluster_names); ?></span></li>
      <?php
      }
      ?>
    </ul>
  </div><!-- /content -->
</div><!-- /page -->
<?php
if ( sizeof($cluster_names) > 1 ) {
?>
 <div data-role="page" class="ganglia-mobile" id="clusters">
  <div data-role="header">
	  <h1>Ganglia Clusters</h1>
  </div>
  <div data-role="content">	
    <ul data-role="listview" data-theme="g">
      <?php
      // List all clusters
      foreach ( $cluster_names as $index => $clustername ) {
	print '<li><a href="#cluster-' . str_replace(" ", "_", $clustername) . '">' . $clustername . '</a></li>';  
      }
      ?>
    </ul>
  </div><!-- /content -->
</div><!-- /page -->
<?php
} // end of if (sizeof(cluster_names))
foreach ( $cluster_names as $index => $clustername ) {
?>
  <div data-role="page" class="ganglia-mobile" id="cluster-<?php print str_replace(" ", "_", $clustername); ?>">
    <div data-role="header">
	    <h1>Cluster <?php print $clustername; ?></h1>
    </div>
    <div data-role="content">	
      <ul data-role="listview" data-theme="g">
	<?php
	// List all hosts in the cluster
	asort($cluster_array[$clustername]);
	foreach ( $cluster_array[$clustername] as $index => $hostname ) {
	  print '<li><a href="mobile_helper.php?show_host_metrics=1&h=' . $hostname . '&c=' . $clustername . '&r=' . $default_time_range . '">' . strip_domainname($hostname) . '</a></li>';  
	}
	?>
      </ul>
    </div><!-- /content -->
  </div><!-- /page -->
<?php
}
///////////////////////////////////////////////////////////////////////////////
// Views
///////////////////////////////////////////////////////////////////////////////
?>
<div data-role="page" class="ganglia-mobile" id="views">
  <div data-role="header">
	  <h1>Ganglia Views</h1>
  </div>
  <div data-role="content">
    <ul data-role="listview" data-theme="g">
      <?php  
      // List all the available views
      foreach ( $available_views as $view_id => $view ) {
	$v = $view['view_name'];
	$elements = get_view_graph_elements($view);
	print '<li><a href="mobile_helper.php?view_name=' . $v . '&just_graphs=1&r=' . $default_time_range . '&cs=&ce=">' . $v . '</a><span class="ui-li-count">' .  sizeof($elements) . '</span></li>';  
      }
      ?>
    </ul>
  </div><!-- /content -->
</div><!-- /page -->
</body>
</html>
