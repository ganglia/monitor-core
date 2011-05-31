<?php

include_once("./eval_conf.php");
include_once("./functions.php");

if ( isset($_GET['action']) && $_GET['action'] == "show_views" ) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  // Show available views
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  $available_views = get_available_views();
  ?>

  <table>
  <?php
  if ( isset($_GET['aggregate']) ) {
  ?>
     <tr><th>Host regular expression</th><td><?php print join (",", $_GET['hreg']); ?></td></tr>
     <tr><th>Metric regular expression</th><td><?php print join (",", $_GET['mreg']); ?></td></tr>
  <?php
    } else {
  ?>
     <tr><th>Hostname</th><td><?php print $_GET['host_name']; ?></td></tr>
     <tr><th>Metric/Report</th><td><?php print $_GET['metric_name']; ?></td></tr>
  <?php
  }
  ?>

  </table>
  <p>
  <form id="add_metric_to_view_form">
    Add to view
    <?php 
    // Get all the aggregate form variables and put them in the hidden fields
    if ( isset($_GET['aggregate']) ) {
	foreach ( $_GET as $key => $value ) {
	  if ( is_array($value) ) {
	    foreach ( $value as $index => $value2 ) {
	      print '<input type=hidden name="' . $key .'[]" value="' . $value2 . '">';
	    }
	  } else {
	    print '<input type=hidden name=' . $key .' value="' . $value . '">';
	  }
	}
    } else {
    ?>
     // If hostname is not set we assume we are dealing with aggregate graphs
    <input type=hidden name=host_name value="<?php print $_GET['host_name']; ?>">
    <input type=hidden name=metric_name value="<?php print $_GET['metric_name']; ?>">
    <input type=hidden name=type value="<?php print $_GET['type']; ?>">
    <?php
    }
    ?>

    <select onChange="addItemToView()" name="view_name">
    <option value='none'>Please choose one</option>
    <?php
    foreach ( $available_views as $view_id => $view ) {
      print "<option value='" . $view['view_name'] . "'>" . $view['view_name'] . "</option>";
    } 

  ?>
    </select>
  </form>
  <form>
    <p>
    Add alert: <p>
    Alert when value is 
    <select name=alert_operator>
      <option value=more>greater</option>
      <option value=less>smaller</option>
      <option value=equal>equal</option>
      <option value=notequal>not equal</option>
    </select> than
    <input size=7 name=critical_value type=text>
    <button onClick="alert('not implemented yet'); return false">Add</button>
  </form>

<?php

} // end of if ( isset($_GET['show_views']) {
?>