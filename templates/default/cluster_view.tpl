<script type="text/javascript">
$(function() {
  // Modified from http://jqueryui.com/demos/toggle/
  //run the currently selected effect
  function runEffect(id){
    //most effect types need no options passed by default
    var options = { };

    options = { to: { width: 200,height: 60 } }; 
    
    //run the effect
    $("#"+id+"_div").toggle("blind",options,500);
  };
 
  //set effect from select menu value
  $('.button').click(function(event) {
    runEffect(event.target.id);
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
      $.get('edit_optional_graphs.php', "clustername={$cluster}", function(data) {
          $('#edit_optional_graphs_content').html(data);
      })
      return false;
    });

    $("#save_optional_graphs_button").click(function(event) {
       $.get('edit_optional_graphs.php', $("#edit_optional_reports_form").serialize(), function(data) {
          $('#edit_optional_graphs_content').html(data);
          $("#save_optional_graphs_button").hide();
          setTimeout(function() {
             $('#edit_optional_graphs').dialog('close');
          }, 5000);
        });
      return false;
    });


});
</script>
<style type="text/css">
  .toggler { width: 500px; height: 200px; }
  a.button { padding: .15em 1em; text-decoration: none; }
  #effect { width: 240px; height: 135px; padding: 0.4em; position: relative; }
  #effect h3 { margin: 0; padding: 0.4em; text-align: center; }
</style>

<div id="metric-actions-dialog" title="Metric Actions">
  <div id="metric-actions-dialog-content">
    Available Metric actions.
  </div>
</div>

<TABLE BORDER="0" CELLSPACING=5 WIDTH="100%">
<TR>
  <TD CLASS=title COLSPAN="2">
  <FONT SIZE="+1">Overview of {$cluster}</FONT>
  </TD>
</TR>

<TR>
<TD ALIGN=left VALIGN=top>
<table cellspacing=1 cellpadding=1 width="100%" border=0>
 <tr><td>CPUs Total:</td><td align=left><B>{$cpu_num}</B></td></tr>
 <tr><td width="60%">Hosts up:</td><td align=left><B>{$num_nodes}</B></td></tr>
 <tr><td>Hosts down:</td><td align=left><B>{$num_dead_nodes}</B></td></tr>
 <tr><td>&nbsp;</td></tr>
 <tr><td colspan=2>Current Load Avg (15, 5, 1m):<br>&nbsp;&nbsp;<b>{$cluster_load}</b></td></tr>
 <tr><td colspan=2>Avg Utilization (last {$range}):<br>&nbsp;&nbsp;<b>{$cluster_util}</b></td></tr>
 <tr><td colspan=2>Localtime:<br>&nbsp;&nbsp;<b>{$localtime}</b></td></tr>
 </table>

{if isset($extra)}
{include(file="$extra")}
{/if}
 <hr>
</TD>
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

<TD ROWSPAN=2 ALIGN="CENTER" VALIGN=top>
<div id="optional_graphs">
  {$optional_reports}<br>
  {foreach $optional_graphs_data graph}
  <A HREF="./graph_all_periods.php?{$graph.graph_args}&amp;g={$graph.name}_report&amp;z=large">
  <IMG BORDER=0 {$additional_cluster_img_html_args} ALT="{$cluster} {$graph.name}" SRC="./graph.php?{$graph.graph_args}&amp;g={$graph.name}_report&amp;z=medium"></A>
  {/foreach}
</div>
{if $user_may_edit}
<button id="edit_optional_graphs_button">Edit Optional Graphs</button>
{/if}
</TD>
</TR>

<TR>
 <TD align=center valign=top>
  <IMG SRC="./pie.php?{$pie_args}" ALT="Pie Chart" BORDER="0">
 </TD>
</TR>
</TABLE>

<script>
// Need to set the field value to metric name
$("#metrics-picker").val("{$metric_name}");
</script>

<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD CLASS=title COLSPAN="2">
  <FONT SIZE="-1">
  Show Hosts Scaled:
  {foreach $showhosts_levels id showhosts implode=""}
  {$showhosts.name}<INPUT type=radio name="sh" value="{$id}" OnClick="ganglia_form.submit();" {$showhosts.checked}>
  {/foreach}
  </FONT>
  |
  {$cluster} <strong>{$metric}</strong>
  last <strong>{$range}</strong>
  sorted <strong>{$sort}</strong>
{if isset($columns_size_dropdown)}
  |
   <FONT SIZE="-1">
   Size&nbsp;&nbsp;{$size_menu}
   Columns&nbsp;&nbsp;{$cols_menu} (0 = metric + reports)
   </FONT>
{/if}
  </TD>
</TR>
</TABLE>

<center>
<table id=graph_sorted_list>
<tr>
{foreach $sorted_list host}
{$host.metric_image}{$host.br}
{/foreach}
</tr>
</table>

{$overflow_list_header}
{foreach $overflow_list host}
{$host.metric_image}{$host.br}
{/foreach}
{$overflow_list_footer}


<p>
{if isset($node_legend)}
(Nodes colored by 1-minute load) | <A HREF="./node_legend.html">Legend</A>{/if}
</CENTER>
