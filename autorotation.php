<?php

include_once("./eval_conf.php");
include_once("./functions.php");

//////////////////////////////////////////////////////////////////////////////////////////
// Print out 
//////////////////////////////////////////////////////////////////////////////////////////
if ( ! isset($_GET['view_name']) ) {

  $available_views = get_available_views();

  print "<form action=autorotation.php><select onchange='this.form.submit();' name=view_name><option value=none>Please choose...</option>";
  foreach ( $available_views as $id => $view ) {
    print "<option value='" . $view['view_name'] . "'>" . $view['view_name'] . "</option>";
  }
  print "</form>";

} else {

  header("Cache-Control: no-cache, must-revalidate");
  header("Expires: Sat, 26 Jul 1997 05:00:00 GMT");

  // We need metrics cache in order to derive cluster name particular host
  // belongs to
  require("./cache.php");

  $available_views = get_available_views();

  // I am not quite sure at this point whether I should cache view info so
  // for now I will have to do this
  foreach ( $available_views as $id => $view ) {
    # Find view settings
    if ( $_GET['view_name'] == $view['view_name'] )
      break;
  }

  unset($available_views);

  if ( sizeof($view['items']) == 0 ) {
      die ("<font color=red size=4>There are no graphs in view '" . $_GET['view_name'] . "'. Please go back and add some.</font>");
  }

  # If timeout is specified use it. Otherwise default to 30 seconds.
  if ( isset($_GET['timeout']))
    $timeout = $_GET['timeout'];
  else
    $timeout = 30;

  # This defines times when updates are happening. For instance if you want
  # to turn off updating during non-business hours you would set 
  # office_hour_min = 8 and office_hour_max = 17. If you want them 24/7
  # set min to 0 and max to 24.
  $office_hour_min = 0;
  $office_hour_max = 24;

  # Graph sizes to use. Those have to be specified in /ganglia/conf.php
  $small_size = "medium";
  $large_size = "xlarge";

  # Path to ganglia. It can be a real URL
  $gangliapath = "graph.php?hc=4&st=";

  # Get the requested graphid and store it in a somewhat more beautiful variable name
  isset($_GET['id']) ? $id = $_GET['id'] : $id = 0;

  // Let's get all View graph elements
  $view_elements = get_view_graph_elements($view);

  # The title of the next graph, with some logic to set the next to the first if we run out of graphs
  if ($id < (count($view_elements) -1)) {
	  $nextid = $id+1;
  } else {
	  $nextid = 0;
  }

  // The title of the graph
  $title = $view_elements[$id]['name'];
  # If it's not an aggregate graph put hostname in the title
  $suffix = isset($view_elements[$nextid]['hostname']) ? " for " . $view_elements[$nextid]['hostname'] : "";
  $nexttitle = $view_elements[$nextid]['name'] . $suffix;

  ?>
  <html>
  <head>
  <title>Ganglia - Graph View</title>
  <meta http-equiv="refresh" content="<?php print "$timeout;url=" . $_SERVER["SCRIPT_NAME"] . "?view_name=" . $_GET['view_name'] ."&id=" . $nextid; ?>&timeout=<?php print $timeout ?>">
  <style>
  body { 
	  margin: 0px;
	  font-family: Tahoma, Helvetica, Verdana, Arial, sans-serif;
  }
  </style>
  </head>


  <body>

  <?php

  $current_hour = date('G');

  #####################################################################################
  # Check whether these are office hours. Display graphs only during office hours
  # so that we can give Ganglia server a little breather from generating all those
  # images
  #####################################################################################
  if ( $current_hour >= $office_hour_min && $current_hour <= $office_hour_max ) {

  ?>

  <div style="position: fixed; left: 20; width: 800; top: 2; font-size: 48px;"><?php echo $title;  ?></div>
  <div style="position: fixed; left: 20; width: 600; top: 55; font-size: 24px;">Next: <?php echo $nexttitle  ?></div><br />

  <table>
  <tr>
    <td><img src="<?php echo $gangliapath . "&r=hour&z=${large_size}&" . $view_elements[$id]['graph_args']; ?>"><br />
	<img src="<?php echo $gangliapath . "&r=day&z=${large_size}&" . $view_elements[$id]['graph_args']; ?>"></td>
    <td valign="top">
      <img src="<?php echo $gangliapath . "&r=week&z=${small_size}&" . $view_elements[$id]['graph_args']; ?>">
      <img src="<?php echo $gangliapath . "&r=month&z=${small_size}&" . $view_elements[$id]['graph_args']; ?>">
    <div style="margin-top: 10px; font-size: 48px; text-align: center;"><?php echo date(DATE_RFC850); ?></div>
    <p>
    <center><form>
    <input type="hidden" name="view_name" value="<?php print $_GET['view_name'] ?>">
    <input type="hidden" name="id" value="<?php print $nextid ?>">
    Rotate graphs every <select onChange="form.submit();" name="timeout">
    <?php
      for ( $i = 10 ; $i <= 90 ; $i += 5 ) {
	if ( $timeout == $i )
	  $selected = "selected";
	else
	  $selected = "";
	print "<option value='" . $i . "' $selected>$i</option>";
      }
    ?>
    </select> seconds.</form></center>
    <p>
    <center><a href="/ganglia/">Go back to Ganglia</a></center></div>
	  </td>

	  </td>
  </tr>
  </table>
  <div>

  <?php

  } else {

  ?>
  <div style="color: red; font-size: 72px;">
  We are sleeping since it's off hours.<p>
  Adjust $office_hour_min and $office_hour_max if this makes you unhappy</h3>
  </div>
  <div style="margin-top: 10px; font-size: 48px; text-align: center;"><?php echo date(DATE_RFC850); ?></div>
  <?php

  }

  print "  </body>
  </html>";

} // end of if (!isset($_GET['view_name']
?>
