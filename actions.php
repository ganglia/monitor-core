<?php

include_once("./conf.php");
include_once("./functions.php");

if ( isset($_GET['action']) && $_GET['action'] == "show_views" ) {
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  // Show available views
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  $available_views = get_available_views();
  ?>

  <table>
  <tr><th>Hostname</th><td><?php print $_GET['host_name']; ?></td></tr>
  <tr><th>Metric/Report</th><td><?php print $_GET['metric_name']; ?></td></tr>
  </table>
  <p>
  <form id="add_metric_to_view_form">
    Add to view
    <input type=hidden name=host_name value="<?php print $_GET['host_name']; ?>">
    <input type=hidden name=metric_name value="<?php print $_GET['metric_name']; ?>">
    <input type=hidden name=type value="<?php print $_GET['type']; ?>">
    <select onChange="addMetricToView()" name="view_name">
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