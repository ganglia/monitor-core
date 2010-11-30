<SCRIPT TYPE="text/javascript"><!--
// Script taken from: http://www.netlobo.com/div_hiding.html
function toggleLayer( whichLayer )
{
  var elem, vis;
  if( document.getElementById ) // this is the way the standards work
    elem = document.getElementById( whichLayer );
  else if( document.all ) // this is the way old msie versions work
      elem = document.all[whichLayer];
  else if( document.layers ) // this is the way nn4 works
    elem = document.layers[whichLayer];
  vis = elem.style;
  // if the style.display value is blank we try to figure it out here
  if(vis.display==''&&elem.offsetWidth!=undefined&&elem.offsetHeight!=undefined)
    vis.display = (elem.offsetWidth!=0&&elem.offsetHeight!=0)?'block':'none';
  vis.display = (vis.display==''||vis.display=='block')?'none':'block';
}
--></SCRIPT>

<script type="text/javascript">
$(function() {
  // Modified from http://jqueryui.com/demos/toggle/
  //run the currently selected effect
  function runEffect(){
    //most effect types need no options passed by default
    var options = { };

    options = { to: { width: 200,height: 60 } }; 
    
    //run the effect
    $("#host_overview").toggle("blind",options,500);
  };
  
  //set effect from select menu value
  $("#button").click(function() {
    runEffect();
    return false;
  });

    $(function() {
	    $( "#edit_optional_graphs" ).dialog({ autoOpen: false, minWidth: 550,
	      beforeClose: function(event, ui) {  location.reload(true); } })
	    $( "#edit_optional_graphs_button" ).button();
	    $( "#save_optional_graphs_button" ).button();
	    $( "#close_edit_optional_graphs_link" ).button();
    });

    $("#edit_optional_graphs_button").click(function(event) {
      $("#edit_optional_graphs").dialog('open');
      $('#edit_optional_graphs_content').html('<img src="img/spinner.gif">');
      $.get('edit_optional_graphs.php', "hostname={$hostname}", function(data) {
	      $('#edit_optional_graphs_content').html(data);
      })
      return false;
    });

    $("#save_optional_graphs_button").click(function(event) {
       $.get('edit_optional_graphs.php', $("#edit_optional_reports_form").serialize(), function(data) {
	      $('#edit_optional_graphs_content').html(data);
	    });
      return false;
    });


});
</script>
<style type="text/css">
  .toggler { width: 500px; height: 200px; }
  #button { padding: .15em 1em; text-decoration: none; }
  #effect { width: 240px; height: 135px; padding: 0.4em; position: relative; }
  #effect h3 { margin: 0; padding: 0.4em; text-align: center; }
</style>

<div id="metric-actions-dialog" title="Metric Actions">
  <div id="metric-actions-dialog-content">
	Available Metric actions.
  </div>
</div>

<div>
<a href="#" id="button" class="ui-state-default ui-corner-all">Host overview</a>
</div>

<div style="display: none;" id=host_overview>
<table>

<TABLE BORDER="0" WIDTH="100%">

<TR>
 <TD ALIGN="LEFT" VALIGN="TOP">

<IMG SRC="{$node_image}" HEIGHT="60" WIDTH="30" ALT="{$host}" BORDER="0">
{$node_msg}
<P>

<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN="2" CLASS=title>Time and String Metrics</TD>
</TR>

{foreach $s_metrics_data s_metric}
<TR>
 <TD CLASS=footer WIDTH="30%">{$s_metric.name}</TD><TD>{$s_metric.value}</TD>
</TR>
{/foreach}

<TR><TD>&nbsp;</TD></TR>

<TR>
  <TD COLSPAN=2 CLASS=title>Constant Metrics</TD>
</TR>

{foreach $c_metrics_data c_metric}
<TR>
 <TD CLASS=footer WIDTH="30%">{$c_metric.name}</TD><TD>{$c_metric.value}</TD>
</TR>
{/foreach}
</TABLE>

 <HR>
{if isset($extra)}
{include(file="$extra")}
{/if}
</TD> 
</table>
</div>
<style>
#edit_optional_graphs_button {
    font-size:12px;
}
#edit_optional_graphs_content {
    padding: .4em 1em .4em 10px;
}
</style>
<div id="edit_optional_graphs">
  <div style="text-align: center;">
    <button  id='save_optional_graphs_button'>Save</button>
  </div>
  <div id="edit_optional_graphs_content">Empty</div>
</div>

<div id="optional_graphs">

<TABLE BORDER="0" WIDTH="100%">

<TR>

<TD ALIGN="CENTER" VALIGN="TOP" WIDTH="395">

{$optional_reports}<br>
<button id="edit_optional_graphs_button">Edit Optional Graphs</button>
</TD>
</TR>
</TABLE>
</div>

<div id="sort_column_dropdowns">
<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD CLASS=title>
  {$host} <strong>graphs</strong>
  last <strong>{$range}</strong>
  sorted <strong>{$sort}</strong>
{if isset($columns_dropdown)}
  <FONT SIZE="-1">
    Columns&nbsp;&nbsp;{$metric_cols_menu}
    Size&nbsp;&nbsp;{$size_menu}
  </FONT>
{/if}
  </TD>
</TR>
</TABLE>

</div>

<div id=metrics>

<CENTER>
<TABLE>
<TR>
 <TD>

{foreach $g_metrics_group_data group g_metrics}
<A HREF="javascript:;" ONMOUSEDOWN="javascript:toggleLayer('{$group}');" TITLE="Toggle {$group} metrics group on/off" NAME="{$group}">
<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD CLASS=metric>
  {$group} metrics ({$g_metrics.group_metric_count})
  </TD>
</TR>
</TABLE>
</A>
<DIV ID="{$group}">
<TABLE><TR>
{foreach $g_metrics["metrics"] g_metric}
<TD>
<a name=metric_{$g_metric.metric_name}>
<font style="font-size: 9px">{$g_metric.metric_name}</font> <a style="background-color: #dddddd" onclick="metricActions('{$g_metric.host_name}','{$g_metric.metric_name}', 'metric'); return false;" href="#">+</a><br>
<A HREF="./graph_all_periods.php?{$g_metric.graphargs}&amp;z=large">
<IMG BORDER=0 ALT="{$g_metric.alt}" SRC="./graph.php?{$g_metric.graphargs}" TITLE="{$g_metric.desc}">
</A>
</TD>
{$g_metric.new_row}
{/foreach}
</TR>
</TABLE>
</DIV>
{/foreach}
 </TD>
</TR>
</TABLE>
</CENTER>

</div>
