<style>
#aggregate_graph_table_form {
   font-size: 12px;
}
</style>
<script>
function createAggregateGraph() {
  $("#aggregate_graph_display").html('<img src="img/spinner.gif">');
  $.get('graph_all_periods.php', $("#aggregate_graph_form").serialize() + "&aggregate=1" , function(data) {
    $("#aggregate_graph_display").html(data);
  });
  return false;
}
var availableMetrics = [ <?php
  require_once('./cache.php');
  foreach ( $index_array['metrics'] as $key => $value)
    $metrics[] = "'" . $key . "'";

  print join(',', $metrics);
  ?>
];

$(function() {
  $( "#metric_chooser" ).autocomplete({
	source: availableMetrics
  });
});
</script>
<div id="aggregate_graph_header">
<h2>Create aggregate graphs</h2>

<form id="aggregate_graph_form">
<table id="aggregate_graph_table_form">
<tr>
<td>Host Regular expression e.g. web-[0,4], web or (web|db):</td>
<td colspan=2><input name="hreg[]" size=60></td>
<tr>
<tr><td>Metric (not a report e.g. load_one, cpu_system):</td>
<td colspan=2><input name="m" id="metric_chooser"></td>
</tr>
<tr>
<td>Graph Type:</td><td>
<div id="graph_type_menu"><input type="radio" name="gtype" value="line" checked>Line</input>
<input type="radio" name="gtype" value="stack">Stacked</input></div></td>
<td>
<input type=submit onclick="createAggregateGraph(); return false" value="Create Graph"></td>
</tr>
</table>
</form>
</div>
<div id="aggregate_graph_display">

</div>