<?php

include_once("./conf.php");
include_once("./functions.php");

// Load the metric caching code
require_once('./cache.php');

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
.ui-mobile #mobile-home {  background: #e5e5e5 url(../images/jqm-sitebg.png) top center repeat-x; }
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
?>
<div data-role="page" class="ganglia-mobile" id="mobile-home">
  <div data-role="header">
    <h1>Ganglia Mobile</h1>
  </div>
  <div data-role="content">
    Please select a category:<p>
    <ul data-role="listview" data-theme="g">
      <li><a href="#views">Views</a></li>
      <?php
	if ( sizeof($cluster_names) == 1) {
           print '<li><a href="#cluster-' . str_replace(" ","_", $cluster_names[0]) . '">Clusters</a></li>';
        } else {
      ?>
      <li><a href="#clusters">Clusters</a></li>
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
	print '<li><a href="#cluster-' . str_replace(" ","_", $clustername) . '">' . $clustername . '</a></li>';  
      }
      ?>
    </ul>
  </div><!-- /content -->
</div><!-- /page -->
<?php
} // end of if (sizeof(cluster_names))
foreach ( $cluster_names as $index => $clustername ) {
?>
  <div data-role="page" class="ganglia-mobile" id="cluster-<?php print str_replace(" ","_", $clustername); ?>">
    <div data-role="header">
	    <h1>Cluster <?php print $clustername; ?></h1>
    </div>
    <div data-role="content">	
      <ul data-role="listview" data-theme="g">
	<?php
	// List all hosts in the cluster
	foreach ( $cluster_array[$clustername] as $index => $hostname ) {
	  print '<li><a href="mobile-helper.php?show_host_metrics=1&h=' . $hostname . '&c=' . $clustername . '&r=' . $default_time_range . '">' . $hostname . '</a></li>';  
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
      $available_views = get_available_views();
  
      # List all the available views
      foreach ( $available_views as $view_id => $view ) {
	$v = $view['view_name'];
	print '<li><a href="mobile-helper.php?view_name=' . $v . '&just_graphs=1&r=' . $default_time_range . '&cs=&ce=">' . $v . '</a></li>';  
      }
      ?>
    </ul>
  </div><!-- /content -->
</div><!-- /page -->
</body>
</html>
