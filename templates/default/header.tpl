<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<HTML>
<HEAD>
<TITLE>Ganglia:: {$page_title}</TITLE>
<META http-equiv="Content-type" content="text/html; charset=utf-8">
<META http-equiv="refresh" content="{$refresh}">
<script TYPE="text/javascript" SRC="js/jquery-1.5.2.min.js"></script>
<script type="text/javascript" src="js/jquery-ui-1.8.11.custom.min.js"></script>
<script type="text/javascript" src="js/jquery.liveSearch.js"></script>
<script type="text/javascript" src="js/ganglia.js"></script>
<script type="text/javascript" src="js/jquery.gangZoom.js"></script>
<script type="text/javascript" src="js/jquery.cookie.js"></script>
<link type="text/css" href="css/smoothness/jquery-ui-1.8.11.custom.css" rel="stylesheet" />
<link type="text/css" href="css/jquery.liveSearch.css" rel="stylesheet" />
<LINK rel="stylesheet" href="./styles.css" type="text/css">
<script>
    var availablemetrics = [ {$available_metrics} ];
    $(function(){
        $( "#metrics-picker" ).autocomplete({
          source: availablemetrics
        });

        {$is_metrics_picker_disabled} 

	$(".submit_button").button();

    });

  $(function () {

    done = function done(startTime, endTime) {
            setStartAndEnd(startTime, endTime);
            document.forms['ganglia_form'].submit();
    }

    cancel = function (startTime, endTime) {
            setStartAndEnd(startTime, endTime);
    }

    defaults = {
        startTime: {$start_timestamp},
        endTime: {$end_timestamp},
        done: done,
        cancel: cancel
    }

    $(".host_small_zoomable").gangZoom($.extend({
        paddingLeft: 67,
        paddingRight: 30,
        paddingTop: 38,
        paddingBottom: 27,
    }, defaults));

    $(".host_default_zoomable").gangZoom($.extend({
        paddingLeft: 66,
        paddingRight: 30,
        paddingTop: 37,
        paddingBottom: 50,
    }, defaults));

    $(".host_large_zoomable").gangZoom($.extend({
        paddingLeft: 66,
        paddingRight: 29,
        paddingTop: 37,
        paddingBottom: 56,
    }, defaults));

    $(".cluster_zoomable").gangZoom($.extend({
        paddingLeft: 67,
        paddingRight: 30,
        paddingTop: 37,
        paddingBottom: 50,
    }, defaults));

    function setStartAndEnd(startTime, endTime) {
        $("#datepicker-cs").val(Math.floor(startTime));
        $("#datepicker-ce").val(Math.floor(endTime));
    }
  });


</script>
{$custom_time_head}
</HEAD>
<BODY BGCOLOR="#FFFFFF">
<style type="text/css">
    body{ font: 75% "Trebuchet MS", sans-serif; margin: 5px;}
</style>

{if $auth_system_enabled}
<div style="float:right">
  {if $username}
    Currently logged in as: {$username} | <a href="logout.php">Logout</a>
  {else}
    You are not currently logged in. | <a href="login.php">Login</a>
  {/if}
</div>
<br style="clear:both"/>
{/if}

<div id="tabs">
<ul>
  <li><a href="#tabs-main">Main</a></li>
  <li><a href="#tabs-search">Search</a></li>
  <li><a href="#tabs-views" onclick="getViewsContent();">Views</a></li>
  <li><a href="aggregate_graphs.php">Aggregate Graphs</a></li>
  <li><a href="#tabs-autorotation" onclick="autoRotationChooser();">Automatic Rotation</a></li>
  <li><a href="#mobile" onclick="location.href = 'mobile.php';">Mobile</a></li>
</ul>

<div id="tabs-main">
<FORM ACTION="{$page}" METHOD="GET" NAME="ganglia_form">
  <TABLE id="table_top_chooser" WIDTH="100%" CELLPADDING="4" CELLSPACING="0" BORDER=0>
  <TR BGCOLOR="#DDDDDD">
     <TD BGCOLOR="#DDDDDD">
     <FONT SIZE="+1">
     <B>{$page_title} for {$date}</B>
     </FONT>
     </TD>
     <TD BGCOLOR="#DDDDDD" ALIGN="RIGHT">
     <INPUT class="submit_button" TYPE="SUBMIT" VALUE="Get Fresh Data">
     </TD>
  </TR>
  <TR>
     <TD COLSPAN="1">
    <div id="range_menu">{$range_menu}{$custom_time}</div>
     </TD>
     <TD>
      <B>{$alt_view}</B>
     </TD>
  </TR>
  <TR>
  <TD COLSPAN="2">
  <div id="sort_menu">
   <B>Metric</B>&nbsp;&nbsp; <input name="m" onclick="$('#metrics-picker').val('');" type=text id="metrics-picker" /><input type="submit" value="Go">&nbsp;&nbsp;
     {$sort_menu}
  </div>
  </TD>
  </TR>

  <tr><td colspan="2">{$node_menu}&nbsp;&nbsp;{$additional_filter_options}</td>
</tr>

  </TABLE>


<HR SIZE="1" NOSHADE>
