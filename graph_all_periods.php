<!DOCTYPE html> 
<html>
<head>
<title>Ganglia: Metric <?php if (isset($_GET['g'])) echo $_GET['g']; else echo $_GET['m']; ?></title>
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
// get arguments are an array rebuild them
foreach ($_GET as $key => $value) {
  if ($key != "r" && $key != "z" && ! is_array($value))
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
?>
<b>Host/Cluster/Aggregate: </b><?php print $description ?>&nbsp;<b>Metric/Graph: </b><?php if (isset($_GET['g'])) echo $_GET['g']; else echo $_GET['m']; ?>
<br />
<img src="graph.php?r=hour&z=<?php print $largesize . $query_string ?>">
<img src="graph.php?r=day&z=<?php print $largesize . $query_string ?>">
<p />

<img src="graph.php?r=week&z=<?php print $largesize . $query_string ?>">
<img src="graph.php?r=month&z=<?php print $largesize . $query_string ?>">

<!--- Scale the yearly image to 100% width --->

<img width=100% src="graph.php?r=year&z=<?php print $xlargesize . $query_string ?>">



</body>
</html>
