<html><head>
<script TYPE="text/javascript" SRC="js/jquery-1.4.4.min.js"></script>
<script type="text/javascript" src="js/jquery-ui-1.8.5.custom.min.js"></script>
<script type="text/javascript" src="js/jquery.liveSearch.js"></script>
<script type="text/javascript" src="js/ganglia.js"></script>
<link type="text/css" href="css/smoothness/jquery-ui-1.8.5.custom.css" rel="stylesheet" />
<link type="text/css" href="css/jquery.liveSearch.css" rel="stylesheet" />
<LINK rel="stylesheet" href="./styles.css" type="text/css">
</head>
<body>
<?php

if ( isset($_GET['host_group'])) {

  require_once('./conf.php');
  // Load the metric caching code
  require_once('./cache.php');

}

?>
<form>
  
  Host Group: <input name="host_group"><br>
  Metric: <input name="metric_name"><br>
  Graph Type:  <input type="radio" name="graph_type" value="line" checked>Line</input>
    <input type="radio" name="graph_type" value="stack">Stacked</input><br>
  <input type=submit>
  <p>
  
</form>

<?php

if ( isset($_GET['metric_name']) )
  $metric_name = $_GET['metric_name'];

isset ($_GET['graph_type']) ? $graph_type = $_GET['graph_type'] : $graph_type = "line";

// Show graph only if host_group is set
if ( isset($_GET['host_group'])) {
  
  print "<img src=graph.php?r=hour&z=xlarge&m=" . $_GET['metric_name'] . "&aggregate=1&hreg[]=" . $_GET['host_group'] . "&gtype=" . $_GET['graph_type'] .  ">";

}

?>

</body>
</html>