<?php

require_once('./conf.php');

$hostname = $_GET['hostname'];

$default_reports = array("included_reports" => array(), "excluded_reports" => array());
$default_file = $GLOBALS['conf_dir'] . "/default.json";
if ( is_file($default_file) ) {
  $default_reports = array_merge($default_reports,json_decode(file_get_contents($default_file), TRUE));
}

$host_file = $GLOBALS['conf_dir'] . "/host_" . $hostname . ".json";
$override_reports = array("included_reports" => array(), "excluded_reports" => array());
if ( is_file($host_file) ) {
  $override_reports = array_merge($override_reports, json_decode(file_get_contents($host_file), TRUE));
}

if ( isset($_GET['action']) ) {

  foreach ( $_GET['reports'] as $report_name => $selection ) {

    $report_name = str_replace("\"", "", $report_name); 

    switch ( $selection ) {
	case "included":
	  # Check if report is already included by default
	  if ( !in_array($report_name, $default_reports["included_reports"] ) ) {
	    $reports["included_reports"][] = $report_name;
	  }
	  break;
	case "excluded":
	  if ( !in_array($report_name, $default_reports["excluded_reports"] ) ) {
	    $reports["excluded_reports"][] = $report_name;
	  }
	  break;
    }
  }

  if ( is_array($reports) ) {
    $json = json_encode($reports);
    $host_file = $GLOBALS['conf_dir'] . "/host_" . $hostname . ".json";
    if ( file_put_contents($host_file, $json) === FALSE ) {
  ?>
	<div class="ui-widget">
			  <div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
				  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
				  <strong>Alert:</strong> Can't write to file <?php print $host_file; ?>. Perhaps permissions are wrong.</p>
			  </div>
	</div>
  <?php
    } else {
  ?>
      <div class="ui-widget">
			  <div class="ui-state-default ui-corner-all" style="padding: 0 .7em;"> 
				  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
				  Change written successfully.</p>
			  </div>
	</div>
  <?php
    }

  } else {

    // Remove file if it already exists since there are no overrides
    if ( is_file($host_file) ) {
      if ( unlink($host_file) !== FALSE ) {
  ?>
      <div class="ui-widget">
			  <div class="ui-state-default ui-corner-all" style="padding: 0 .7em;"> 
				  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
				  Change written successfully.</p>
			  </div>
	</div>
  <?php
      } else {
  ?>
	<div class="ui-widget">
			  <div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
				  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
				  <strong>Alert:</strong> Can't write to file <?php print $host_file; ?>. Perhaps permissions are wrong.</p>
			  </div>
	</div>
  <?php
      }

    } else {
  ?>
      <div class="ui-widget">
			  <div class="ui-state-default ui-corner-all" style="padding: 0 .7em;"> 
				  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
				  Change written successfully.</p>
			  </div>
	</div>
  <?php


    } // end of if ( is_file($host_file) ) {

  } // end of if ( is_array($reports)

} else {

?>

<form id=edit_optional_reports_form>
<input type=hidden name=hostname value=<?php print $_GET['hostname']; ?>>
<input type=hidden name=action value=change>
<table border=1 width=90%>
<style>
.checkboxes {
  text-align: center;
}
</style>
<?php

print "<h4>Hostname: " . $hostname . "</h4><br>";
?>
<?php
function create_radio_button($variable_name, $variable_value = "ignored") {
  $str = "";
  $possible_values = array("included", "excluded", "ignored" );
  foreach ( $possible_values as $key => $value ) {
    $variable_value == $value ? $checked = "checked" : $checked = "";
    $str .=  "<td><input type=radio value=" . $value . " name=reports[\"" . $variable_name . "\"] $checked></td>";    
  }

  return $str;
}


/* -----------------------------------------------------------------------
 Find available graphs by looking in the GANGLIA_DIR/graph.d directory
 anything that matches _report.php
----------------------------------------------------------------------- */
if ($handle = opendir($GLOBALS['ganglia_dir'] . '/graph.d')) {

    // If we are using Graphite reports are in JSON files instead of the standard PHP files
    if ( $use_graphite == "yes" )
      $report_suffix = "json";
    else
      $report_suffix = "php";

    while (false !== ($file = readdir($handle))) {
      if ( preg_match("/(.*)(_report)\.(" . $report_suffix .")/", $file, $out) ) {
        $available_reports[] = $out[1] . "_report";
      }
    }

    closedir($handle);
}

asort($available_reports);
?>

<tr>
<th>Explicitly Included</th>
<th>Explicitly Excluded</th>
<th>Ignored</th>
<th>Report Name</th>
</tr>
<?php

foreach ( $available_reports as $key => $report_name ) {
  print "<tr class=checkboxes>";
    $report_value = "ignored";
    if ( in_array($report_name, $default_reports['included_reports']) || in_array($report_name, $override_reports['included_reports']) )
      $report_value = "included";
    if  ( in_array($report_name, $default_reports['excluded_reports']) || in_array($report_name, $override_reports['excluded_reports']) )
      $report_value = "excluded";

    print create_radio_button($report_name, $report_value) ."<td>" . $report_name . "</td>";
  print "</tr>";
}

?>

</table>
</form>

<?php

} // if ( isset($_GET['action') )

?>