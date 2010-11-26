<?php

include_once("./conf.php");
include_once("./functions.php");

?>
<!DOCTYPE html> 
<html> 
<head> 
<title>Ganglia Mobile</title> 
<link rel="stylesheet" href="http://code.jquery.com/mobile/1.0a2/jquery.mobile-1.0a2.min.css" />
<script src="http://code.jquery.com/jquery-1.4.4.min.js"></script>
<script src="http://code.jquery.com/mobile/1.0a2/jquery.mobile-1.0a2.min.js"></script>
<style>
.ui-mobile #jqm-home {  background: #e5e5e5 url(../images/jqm-sitebg.png) top center repeat-x; }
.ui-mobile #jqm-homeheader { padding: 55px 25px 0; text-align: center }
.ui-mobile #jqm-homeheader h1 { margin: 0 0 10px; }
.ui-mobile #jqm-homeheader p { margin: 0; }
.ui-mobile #jqm-version { text-indent: -99999px; background: url(../images/version.png) top right no-repeat; width: 119px; height: 122px; overflow: hidden; position: absolute; top: 0; right: 0; }
.ui-mobile .jqm-themeswitcher { clear: both; margin: 20px 0 0; }

h2 { margin-top:1.5em; }
p code { font-size:1.2em; font-weight:bold; } 

dt { font-weight: bold; margin: 2em 0 .5em; }
dt code, dd code { font-size:1.3em; line-height:150%; }
</style>

</head> 
<body> 

<!-- Start of first page -->
<div data-role="page" id="jqm-home">

  <div data-role="header">
	  <h1>Ganglia Mobile Views</h1>
  </div><!-- /header -->

  <div data-role="content">	
  <ul data-role="listview" data-theme="g">
    <?php
    $available_views = get_available_views();

    # List all the available views
    foreach ( $available_views as $view_id => $view ) {
      $v = $view['view_name'];
      print '<li><a href="views-mob.php?view_name=' . $v . '&just_graphs=1&r=hour&cs=&ce=">' . $v . '</a></li>';  
    }

    ?>
  </ul>
	</div><!-- /content -->

</div><!-- /page -->


<!-- Start of second page -->
<div data-role="page" id="bar">

	<div data-role="header">
		<h1>Bar</h1>
	</div><!-- /header -->

	<div data-role="content">	
		<p>I'm first in the source order so I'm shown as the page.</p>		
		<p><a href="#jqm-home">Back to foo</a></p>	
	</div><!-- /content -->

	<div data-role="footer">	
  <div data-role="navbar">
    <ul>
      <li><a href="a.html" class="ui-btn-active">One</a></li>
      <li><a href="b.html">Two</a></li>
    </ul>
  </div><!-- /navbar -->
	</div><!-- /footer -->
</div><!-- /page -->
</body>
</html>
