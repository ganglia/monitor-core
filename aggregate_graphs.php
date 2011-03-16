<script>
function createAggregateGraph() {
  $("#ag_graph_display").html('<img src="img/spinner.gif">');
  $.get('graph_all_periods.php', $("#aggregate_graph_form").serialize() + "&aggregate=1" , function(data) {
    $("#aggregate_graph_display").html(data);
  });
  return false;
}

</script>
<h2>Create aggregate graphs</h2>

<form id="aggregate_graph_form">
<table id="aggregate_graph_table_form">
<tr>
<td>Host Regular expression e.g. web-[0,4], web or (web|db):</td>
<td><input name="hreg[]"></td>
<tr>
<tr><td>Metric (not a report e.g. load_one, cpu_system):</td>
<td><input name="m"></td>
</tr>
<tr>
<td>Graph Type:</td><td>
<div id="graph_type_menu"><input type="radio" name="gtype" value="line" checked>Line</input>
<input type="radio" name="gtype" value="stack">Stacked</input></div></td>
</tr>
<tr>
<td colspan=2>
<input type=submit onclick="createAggregateGraph(); return false" value="Create Graph">
</td></tr>
</table>
<p>  
</form>

<div id="aggregate_graph_display">

</div>