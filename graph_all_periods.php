<!DOCTYPE html> 
<html>
<head>
<title>Ganglia: Graph all periods</title>
<style>
.img_view {
  float: left;
  margin: 0 0 10px 10px;
}
</style>
<?php
include_once "./eval_conf.php";

// build a query string but drop r and z since those designate time window and size. Also if the 
// get arguments are an array rebuild them. For example with hreg (host regex)
$ignore_keys_list = array("r", "z", "st", "cs", "ce", "hc");

foreach ($_GET as $key => $value) {
  if ( ! in_array($key, $ignore_keys_list) && ! is_array($value))
    $query_string_array[] = "$key=$value";

  // $_GET argument is an array. Rebuild it to pass it on
  if ( is_array($value) ) {
    foreach ( $value as $index => $value2 )
      $query_string_array[] = $key . "[]=" . $value2;

  }
}

// If we are in the mobile mode set the proper graph sizes
if ( isset($_GET['mobile'])) {
  $largesize = "mobile";
  $xlargesize = "mobile";
} else {
  $largesize = "large";
  $xlargesize = "xlarge";  
}

// Join all the query_string arguments
$query_string = "&" . join("&", $query_string_array);

// Descriptive host/aggregate graph
if (isset($_GET['h']))
  $description = $_GET['h'];
else if (isset($_GET['c']))
  $description = $_GET['c'];
else if (is_array($_GET['hreg']) )
  $description = join(",", $_GET['hreg']);
else
  $description = "Unknown";

if (isset($_GET['g'])) 
  $metric_description = $_GET['g'];
else if ( isset($_GET['m'] ))
  $metric_description = $_GET['m'];
else if (is_array($_GET['mreg']) )
  $metric_description = join(",", $_GET['mreg']);
else
  $metric_description = "Unknown";


if ( $conf['graph_engine'] == "flot" ) {
?>
<style>
.flotgraph {
  height: <?php print $conf['graph_sizes'][$largesize]["height"] ?>px;
  width:  <?php print $conf['graph_sizes'][$largesize]["width"] ?>px;
}
</style>
<?php
// Add JQuery and flot loading only if this is not embedded in the Aggregate Graphs
// Tab
if ( ! isset($_GET['embed'] ) ) {
?>
<!--[if lte IE 8]><script language="javascript" type="text/javascript" src="js/excanvas.min.js"></script><![endif]-->
<script language="javascript" type="text/javascript" src="js/jquery-1.5.2.min.js"></script>
<script language="javascript" type="text/javascript" src="js/jquery.flot.min.js"></script>
<?php
} // end of if ( ! isset($_GET['embed'] )
?>
<script type="text/javascript">
  var default_time = 'hour';
  var metric = "<?php if (isset($_GET['g'])) echo $_GET['g']; else echo $_GET['m']; ?>";
  var base_url = "<?php print 'graph.php?flot=1&' . $_GET['m'] . $query_string ?>" + "&r=" + default_time;
</script>
<script type="text/javascript" src="js/create-flot-graphs.js"></script>
<?php
} // end of if ( $conf['graph_engine'] == "flot" ) {
?>
</head>

<body>
<?php
if ( isset($_GET['mobile'])) {
?>
    <div data-role="page" class="ganglia-mobile" id="view-home">
    <div data-role="header">
      <a href="#" class="ui-btn-left" data-icon="arrow-l" onclick="history.back(); return false">Back</a>
      <h3><?php if (isset($_GET['g'])) echo $_GET['g']; else echo $_GET['m']; ?></h3>
      <a href="#mobile-home">Home</a>
    </div>
    <div data-role="content">
<?php
}

// Skip printing if this is an aggregate graphs 
if ( ! isset($_GET['embed'] )  ) {
?>
<b>Host/Cluster/Host Regex: </b><?php print $description ?>&nbsp;<b>Metric/Graph/Metric Regex: </b><?php 
  print $metric_description; 
?><br />
<?php
}

foreach ( $conf['time_ranges'] as $key => $value ) {

   print '<div class="img_view">' .
  '<a href="./graph.php?r=' . $key . $query_string .'&csv=1"><img title="Export to CSV" height=16 width=16 src="img/csv.png"></a> ' .
  '<a href="./graph.php?r=' . $key . $query_string .'&json=1"><img title="Export to JSON" height=16 width=16 src="img/js.png"></a>' .
  '<br />';

  // If we are using flot we need to use a div instead of an image reference
  if ( $conf['graph_engine'] == "flot" ) {

    print '<div id="placeholder_' . $key . '" class="flotgraph img_view"></div>';

  } else {

    print '<a href="./graph.php?r=' . $key . '&z=' . $xlargesize . $query_string . '"><img alt="Last ' . $key . '" src="graph.php?r=' . $key . '&z=' . $largesize . $query_string . '"></a>';

  }

  print "</div>";

}
// The div below needs to be added to clear float left since in aggregate view things
// will start looking goofy
?>
<div style="clear: left"></div>
</body>
</html>
