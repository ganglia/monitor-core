<html>
<head>
<title>Ganglia: Metric <?php echo $_GET['g']; ?> for <?php echo $_GET['h'] ?></title>
</head>
<body>
<?php

$query_string = "";

# build a query string but drop r and z since those designate time window and size
foreach ($_GET as $key => $value) {
  if ($key != "r" && $key != "z")
    $query_string .= "&$key=$value";
}

?>
<b>Host: </b><?php echo $_GET['h'] ?>&nbsp;<b>Metric/Graph: </b><?php if (isset($_GET['g'])) echo $_GET['g']; else echo $_GET['m']; ?>
<br />

<img src="graph.php?r=hour&z=large<?php echo $query_string ?>">
<img src="graph.php?r=day&z=large<?php echo $query_string ?>">
<p />

<img src="graph.php?r=week&z=large<?php echo $query_string ?>">
<img src="graph.php?r=month&z=large<?php echo $query_string ?>">

<!--- Scale the yearly image to 100% width --->

<img width=100% src="graph.php?r=year&z=extralarge<?php echo $query_string ?>">
</body>
</html>
