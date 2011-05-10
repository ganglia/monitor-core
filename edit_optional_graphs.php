<?php

//////////////////////////////////////////////////////////////////////////
// This file generates the Edit Optional Graphs dialog. 
// Depending whether we are editing hostname or clusters filenames
// are going start with either host_ or cluster_
//////////////////////////////////////////////////////////////////////////
require_once './eval_conf.php';
require_once 'functions.php';
// Load the host cache for validation purposes
include_once './cache.php';

$hostname = isset($_GET['hostname'])  ? sanitize( $_GET['hostname'] ) : "none";
if ( $hostname != "none" ) {
    $json_filename_suffix = "/host_" . $hostname . ".json";
    $clustername = $index_array['cluster'][$hostname];
} else if ( $clustername != "none" ) {
    // TODO Add validation for clusters
    $clustername = isset($_GET['clustername'])  ? sanitize( $_GET['clustername'] ) : "none";
    $json_filename_suffix = "/cluster_" . str_replace(" ", "_", $clustername) . ".json";
}

$error = false;
if( ! checkAccess( $clustername, GangliaAcl::EDIT, $conf ) ) {
  $error = "You do not have permission to make changes in cluster '$clustername'.";
// Does host exist in the metric cache. If it doesn't something's wrong
} else if ( ! isset($index_array['cluster'][$hostname] ) && $hostname != "none" ) {
  $error = "I can't find the hostname <strong>$hostname</strong> you supplied. It's either invalid or this host hasn't made it into the cache yet.";
}

if( $error ) {
?>
  <div class="ui-widget">
    <div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
      <p>
        <span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
        <strong>Alert: <?php echo $error ?></strong> 
      </p>
    </div>
  </div>
<?php  
  exit(1);
}


$default_reports = array("included_reports" => array(), "excluded_reports" => array());
$default_file = $conf['conf_dir'] . "/default.json";
if ( is_file($default_file) ) {
  $default_reports = array_merge($default_reports,json_decode(file_get_contents($default_file), TRUE));
}

$override_file = $conf['conf_dir'] . $json_filename_suffix;
$override_reports = array("included_reports" => array(), "excluded_reports" => array());
if ( is_file($override_file) ) {
  $override_reports = array_merge($override_reports, json_decode(file_get_contents($override_file), TRUE));
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
    if ( file_put_contents($override_file, $json) === FALSE ) {
  ?>
    <div class="ui-widget">
              <div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
                  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
                  <strong>Alert:</strong> Can't write to file <?php print $file; ?>. Perhaps permissions are wrong.</p>
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
    if ( is_file($file) ) {
      if ( unlink($file) !== FALSE ) {
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
                  <strong>Alert:</strong> Can't write to file <?php print $file; ?>. Perhaps permissions are wrong.</p>
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


    } // end of if ( is_file($file) ) {

  } // end of if ( is_array($reports)

} else {

?>

<form id=edit_optional_reports_form>
<input type=hidden name=hostname value="<?php print $hostname ?>">
<input type=hidden name=clustername value="<?php print $clustername; ?>">
<input type=hidden name=action value=change>
<table border=1 width=90%>
<style>
.checkboxes {
  text-align: center;
}
</style>
<?php

print "<h4>Hostname: " . $hostname . "</h4>";
print "<h4>Clustername: " . $clustername . "</h4>";
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

// Initialize the available reports array
$available_reports = array();

/* -----------------------------------------------------------------------
 Find available graphs by looking in the GANGLIA_DIR/graph.d directory
 anything that matches _report.php
----------------------------------------------------------------------- */
if ($handle = opendir($conf['ganglia_dir'] . '/graph.d')) {

    // If we are using RRDtool reports can be json or PHP suffixes
    if ( $conf['graph_engine'] == "rrdtool" )
      $report_suffix = "php|json";
    else
      $report_suffix = "json";

    while (false !== ($file = readdir($handle))) {
      if ( preg_match("/(.*)(_report)\.(" . $report_suffix .")/", $file, $out) ) {
        if ( ! in_array($out[1] . "_report", $available_reports) )
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
