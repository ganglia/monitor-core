<?php

include_once("./conf.php");
include_once("./functions.php");
//////////////////////////////////////////////////////////////////////////////////////////////////////
// Create new view
//////////////////////////////////////////////////////////////////////////////////////////////////////
if ( isset($_GET['create_view']) ) {
  // Check whether the view name already exists
  $view_exists = 0;

  $available_views = get_available_views();

  foreach ( $available_views as $view_id => $view ) {
    if ( $view['view_name'] == $_GET['view_name'] ) {
      $view_exists = 1;
    }
  }

  if ( $view_exists == 1 ) {
  ?>
      <div class="ui-widget">
	<div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
	  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
	  <strong>Alert:</strong> View with the name <?php print $_GET['view_name']; ?> already exists.</p>
	</div>
      </div>
    <?php
  } else {
    $empty_view = array ( "view_name" => $_GET['view_name'],
      "items" => array() );
    $view_suffix = str_replace(" ", "_", $_GET['view_name']);
    $view_filename = $GLOBALS['views_dir'] . "/view_" . $view_suffix . ".json";
    $json = json_encode($empty_view);
    if ( file_put_contents($view_filename, $json) === FALSE ) {
    ?>
      <div class="ui-widget">
	<div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
	  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
	  <strong>Alert:</strong> Can't write to file <?php print $view_filename; ?>. Perhaps permissions are wrong.</p>
	</div>
      </div>
    <?php
    } else {
    ?>
      <div class="ui-widget">
	<div class="ui-state-default ui-corner-all" style="padding: 0 .7em;"> 
	  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
	  View has been created successfully.</p>
	</div>
	</div>
    <?php
    } // end of if ( file_put_contents($view_filename, $json) === FALSE ) 
  }  // end of if ( $view_exists == 1 )
  exit(1);
} 

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Create new view
//////////////////////////////////////////////////////////////////////////////////////////////////////
if ( isset($_GET['add_to_view']) ) {

  $view_exists = 0;
  // Check whether the view name already exists
  $available_views = get_available_views();

  foreach ( $available_views as $view_id => $view ) {
    if ( $view['view_name'] == $_GET['view_name'] ) {
      $view_exists = 1;
      break;
    }
  }

  if ( $view_exists == 0 ) {
  ?>
      <div class="ui-widget">
	<div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
	  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
	  <strong>Alert:</strong> View <?php print $_GET['view_name']; ?> does not exist. This should not happen.</p>
	</div>
      </div>
    <?php
  } else {

    // Read in contents of an existing view
    $view_filename = $view['file_name'];
    // Delete the file_name index
    unset($view['file_name']);

    if ( $_GET['type'] == "metric" ) 
      $view['items'][] = array( "hostname" => $_GET['host_name'], "metric" => $_GET['metric_name']);
    else
      $view['items'][] = array( "hostname" => $_GET['host_name'], "graph" => $_GET['metric_name']);

    $json = json_encode($view);

    if ( file_put_contents($view_filename, $json) === FALSE ) {
    ?>
      <div class="ui-widget">
	<div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
	  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
	  <strong>Alert:</strong> Can't write to file <?php print $view_filename; ?>. Perhaps permissions are wrong.</p>
	</div>
      </div>
    <?php
    } else {
    ?>
      <div class="ui-widget">
	<div class="ui-state-default ui-corner-all" style="padding: 0 .7em;"> 
	  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
	  View has been updated successfully.</p>
	</div>
	</div>
    <?php
    } // end of if ( file_put_contents($view_filename, $json) === FALSE ) 
  }  // end of if ( $view_exists == 1 )
  exit(1);
} 



// Load the metric caching code we use if we need to display graphs
require_once('./cache.php');

$available_views = get_available_views();

// Pop up a warning message if there are no available views
if ( sizeof($available_views) == 0 ) {
    ?>
	<div class="ui-widget">
			  <div class="ui-state-error ui-corner-all" style="padding: 0 .7em;"> 
				  <p><span class="ui-icon ui-icon-alert" style="float: left; margin-right: .3em;"></span> 
				  <strong>Alert:</strong> There are no views defined.</p>
			  </div>
	</div>
  <?php
} else {

  if ( !isset($_GET['view_name']) ) {
    if ( sizeof($available_views) == 1 )
      $view_name = $available_views[0]['view_name'];
    else
      $view_name = "default";
  } else {
    $view_name = $_GET['view_name'];
  }

  if ( isset($_GET['standalone']) ) {
    ?>
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
  <div id="tabs-views-content">
    <?php
  }

  print "<form id=view_chooser_form>";
  
  if ( ! isset($_GET['just_graphs']) ) {

  ?>
    <table id=views_table>
    <tr><td valign=top>

  <?php
    if ( ! isset($_GET['standalone']) ) {
  ?>
      <button onclick="return false" id=create_view_button>Create View</button>
      <a href="views.php?standalone=1" id="detach-tab-button">Detach Tab</a> 
  <?php
   }
  ?>
    <p>  <div id="views_menu">
      Existing views:
      <ul id="navlist">
    <?php

    # List all the available views
    foreach ( $available_views as $view_id => $view ) {
      $v = $view['view_name'];
      print '<li><a href="#" onClick="getViewsContentJustGraphs(\'' . $v . '\', \'1hour\', \'\',\'\'); return false;">' . $v . '</a></li>';  
    }

    ?>
<script>
$(function(){
    $( "#view_range_chooser" ).buttonset();
    $( "#detach-tab-button").button();
    document.getElementById('view_name').value = "default";
});
</script>


    </ul></div></td><td valign=top>
    <div id=view_range_chooser>
    <form id=view_timerange_form>
    <input type="hidden" name=view_name id=view_name value="">
<?php
   $context_ranges = array_keys( $time_ranges );
   if (isset($jobrange))
      $context_ranges[]="job";
   if (isset($cs) or isset($ce))
      $context_ranges[]="custom";

   if ( isset($_GET['r']) ) 
    $range = $_GET['r'];
   else
    $range = "";

   $range_menu = "<B>Last</B>&nbsp;&nbsp;";
   foreach ($context_ranges as $v) {
      $url=rawurlencode($v);
      if ($v == $range)
	$checked = "checked=\"checked\"";
      else
	$checked = "";
#	$range_menu .= "<input OnChange=\"getViewsContentJustGraphs(document.getElementById('view_name').value);\" type=\"radio\" id=\"view-range-$v\" name=\"r\" value=\"$v\" $checked/><label for=\"view-range-$v\">$v</label>";
      $range_menu .= "<input OnChange=\"document.getElementById('view-cs').value = ''; document.getElementById('view-ce').value = ''; getViewsContentJustGraphs(document.getElementById('view_name').value, '" . $v . "', '','');\" type=\"radio\" id=\"view-range-$v\" name=\"r\" value=\"$v\" $checked/><label for=\"view-range-$v\">$v</label>";

   }
  print $range_menu;
?>
      &nbsp;&nbsp;or from 
  <INPUT TYPE="TEXT" TITLE="Feb 27 2007 00:00, 2/27/2007, 27.2.2007, now -1 week, -2 days, start + 1 hour, etc." NAME="cs" ID="view-cs" SIZE="17"> to 
  <INPUT TYPE="TEXT" TITLE="Feb 27 2007 00:00, 2/27/2007, 27.2.2007, now -1 week, -2 days, start + 1 hour, etc." NAME="ce" ID="view-ce" SIZE="17"> 
  <input type="button" onclick="getViewsContentJustGraphs(document.getElementById('view_name').value, '', document.getElementById('view-cs').value, document.getElementById('view-ce').value ); return false;" value="Go">
  <input type="button" value="Clear" onclick="document.getElementById('view-cs').value = ''; document.getElementById('view-ce').value = '' ; return false;">
      </form><p>
      </div>
    </div>

  <?php

  } // end of  if ( ! isset($_GET['just_graphs']) 

  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  // Displays graphs in the graphs div
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
?>
<!DOCTYPE html> 
<html> 
<head> 
<title>Ganglia Mobile</title> 
</head>
<body>
<div data-role="page" id="jqm-home">
  <div data-role="header">
	  <h1>View <?php print $_GET['view_name']; ?></h1>
	<div data-role="navbar">
		<ul>
<?php

   $my_ranges = array_keys( $time_ranges );   

   for ( $i = 0 ; $i < 5 ; $i++ ) {
      $context_ranges[] = $my_ranges[$i]; 
   }

   $range_menu = "";

   foreach ($context_ranges as $v) {
      $url=rawurlencode($v);
      if ($v == $range)
	$checked = "class=\"ui-btn-active\"";
      else
	$checked = "";

      $range_menu .= "<li><a href='views-mob.php?view_name=" . $_GET['view_name'] . "&just_graphs=1&r=" . $v . "&cs=&ce='>$v</a></li>";
   }
    print $range_menu;
?>
		</ul>
	</div><!-- /navbar -->
  </div><!-- /header -->

  <div data-role="content">	
	</div><!-- /content -->
<?php


  // Let's find the view definition
  foreach ( $available_views as $view_id => $view ) {

   if ( $view['view_name'] == $view_name ) {

      $view_elements = get_view_graph_elements($view);

      $range_args = "";
      if ( isset($_GET['r']) && $_GET['r'] != "" ) 
	    $range_args .= "&r=" . $_GET['r'];
      if ( isset($_GET['cs']) && isset($_GET['ce']) ) 
	    $range_args .= "&cs=" . $_GET['cs'] . "&ce=" . $_GET['ce'];

      foreach ( $view_elements as $id => $element ) {

	  print "
	  <A HREF=\"./graph_all_periods.php?" . $element['graph_args'] ."&z=mobile\">
	  <IMG ALT=\"" . $element['hostname'] . " - " . $element['name'] . "\" BORDER=0 SRC=\"./graph.php?" . $element['graph_args'] . "&z=mobile" . $range_args .  "\"></A>";
      }

   }  // end of if ( $view['view_name'] == $view_name
  } // end of foreach ( $views as $view_id 

  print "</div>"; 

  if ( ! isset($_GET['just_graphs']) )
    print "</td></tr></table></form>";

  if ( isset($_GET['standalone']) ) {
    print "</div>";
  }


} // end of ie else ( ! isset($available_views )

?>
