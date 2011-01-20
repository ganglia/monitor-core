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

$colors = array("128936","FF8000","158499","CC3300","996699","FFAB11","3366CC","01476F");
$color_count = sizeof($colors);

$counter = 0;

isset ($_GET['graph_type']) ? $graph_type = $_GET['graph_type'] : $graph_type = "line";

// Show graph only if host_group is set
if ( isset($_GET['host_group'])) {

  $graph = array ("report_name" => $metric_name, "report_type" => "standard", "title" => $metric_name . " Report", "vertical_label" => " ");

  $query = $_GET['host_group'];

  foreach ( $index_array['hosts'] as $key => $host_name ) {
    if ( preg_match("/$query/i", $host_name ) ) {
      $cluster_name = $index_array['cluster'][$host_name];

      // We need to cycle the available colors
      $color_index = $counter % $color_count; 

      $graph['series'][] = array ( "hostname" => $host_name , "clustername" => $cluster_name,
	"metric" => $metric_name,  "color" => $colors[$color_index], "label" => $host_name, "line_width" => "2", "type" => $graph_type);
      $counter++;
      
    }
  }
  
  $json = urlencode(serialize($graph));
  
  print "<img src=graph.php?r=hour&z=xlarge&m=" . $_GET['metric_name'] . "&graph_config=" . $json . ">";

}

?>

</body>
</html>