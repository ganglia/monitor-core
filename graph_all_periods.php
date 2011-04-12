<!DOCTYPE html> 
<html>
<head>
<title>Ganglia: Metric <?php if (isset($_GET['g'])) echo $_GET['g']; else echo $_GET['m']; ?></title>
<style>
.img_view {
  float: left;
  margin: 0 0 10px 10px;
}
</style>
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

$query_string = "";

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

if (isset($_GET['h']))
  $description = $_GET['h'];
else if (isset($_GET['c']))
  $description = $_GET['c'];
else if (isset($_GET['aggregate']) )
  $description = "Aggregate graph";
else
  $description = "Unknown";

// Skip printing the 
if ( ! isset($_GET['aggregate'] )  ) {
?>
<b>Host/Cluster/Aggregate: </b><?php print $description ?>&nbsp;<b>Metric/Graph: </b><?php if (isset($_GET['g'])) echo $_GET['g']; else echo $_GET['m']; ?><br />
<?php
}

include_once "./eval_conf.php";

foreach ( $conf['time_ranges'] as $key => $value ) {

  print '<div class="img_view">' .
  '<a href="./graph.php?r=' . $key . $query_string .'&csv=1"><img alt="Export to CSV" height=16 width=16 src="img/csv.png"></a> ' .
  '<a href="./graph.php?r=' . $key . $query_string .'&json=1"><img alt="Export to JSON" height=16 width=16 src="img/js.png"></a>' .
  '<br /><img alt="Last ' . $key . '" src="graph.php?r=' . $key . '&z=' . $largesize . $query_string . '"></div>';

}
// The div below needs to be added to clear float left since in aggregate view things
// will start looking goofy
?>
<div style="clear: left"></div>
</body>
</html>
