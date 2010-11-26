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

<TD ROWSPAN=2 ALIGN="CENTER" VALIGN=top>
<A HREF="./graph.php?{$graph_args}&amp;g=load_report&amp;z=large">
<IMG BORDER=0 ALT="{$cluster} LOAD"
   SRC="./graph.php?{$graph_args}&amp;g=load_report&amp;z=medium">
</A>
<A HREF="./graph.php?{$graph_args}&amp;g=mem_report&amp;z=large">
<IMG BORDER=0 ALT="{$cluster} MEM"
   SRC="./graph.php?{$graph_args}&amp;g=mem_report&amp;z=medium">
</A>
<A HREF="./graph.php?{$graph_args}&amp;g=cpu_report&amp;z=large">
<IMG BORDER=0 ALT="{$cluster} CPU"
   SRC="./graph.php?{$graph_args}&amp;g=cpu_report&amp;z=medium">
</A>
<A HREF="./graph.php?{$graph_args}&amp;g=network_report&amp;z=large">
<IMG BORDER=0 ALT="{$cluster} NETWORK"
    SRC="./graph.php?{$graph_args}&amp;g=network_report&amp;z=medium">
</A>
{foreach $optional_graphs_data graph}
<A HREF="./graph.php?{$graph.graph_args}&amp;g={$graph.name}_report&amp;z=large">
<IMG BORDER=0 ALT="{$cluster} {$graph.name}" SRC="./graph.php?{$graph.graph_args}&amp;g={$graph.name}_report&amp;z=medium">
</A>
{/foreach}
</TD>
</TR>

<TR>
 <TD align=center valign=top>
  <IMG SRC="./pie.php?{$pie_args}" ALT="Pie Chart" BORDER="0">
 </TD>
</TR>
</TABLE>


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

<CENTER>
<TABLE>
<TR>
{foreach $sorted_list host}
{$host.metric_image}{$host.br}
{/foreach}
</TR>
</TABLE>

<p>
{if isset($node_legend)}
(Nodes colored by 1-minute load) | <A HREF="./node_legend.html">Legend</A>{/if}
</CENTER>
