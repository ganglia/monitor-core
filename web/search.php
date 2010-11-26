<?php

require_once('./conf.php');

// Load the metric caching code
require_once('./cache.php');

$results = "";

if ( isset($_GET['q']) && $_GET['q'] != "" ) {

  $query = $_GET['q'];
  // First we look for the hosts
  foreach ( $index_array['hosts'] as $key => $host_name ) {
    if ( preg_match("/$query/", $host_name ) ) {
      $cluster_name = $index_array['cluster'][$host_name];
      $results .= "Host: <a target=\"_blank\" href=\"?c=" . $cluster_name . "&h=" . $host_name . "&m=cpu_report&r=hour&s=descending&hc=4&mc=2\">" . $host_name . "</a><br>";
    }
  }

  // Now let's look through metrics.
  foreach ( $index_array['metrics'] as $metric_name => $hosts ) {
    if ( preg_match("/$query/", $metric_name ) ) {
      foreach ( $hosts as $key => $host_name ) {
	$cluster_name = $index_array['cluster'][$host_name];
	$results .= "Metric: <a target=\"_blank\" href=\"?c=" . $cluster_name . "&h=" . $host_name . "&m=cpu_report&r=hour&s=descending&hc=4&mc=2#metric_" . $metric_name  . "\">" . $host_name . " (" . $metric_name .  " )</a><br>";
      }
    }
  }

} else {
  $results .= "Empty query string";
}

if ( $results == "" ) {
  print "No results. Try a different search term. Currently only one search term supported.";
} else {
  print $results;
}

?>
