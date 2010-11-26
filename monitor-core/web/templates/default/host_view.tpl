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
<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN="2" BGCOLOR="#EEEEEE" ALIGN="CENTER">
  <FONT SIZE="+2">{$host} Overview</FONT>
  </TD>
</TR>

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

<TD ALIGN="CENTER" VALIGN="TOP" WIDTH="395">
<A HREF="./graph.php?{$graphargs}&amp;g=load_report&amp;z=large&amp;c={$cluster_url}">
<IMG BORDER=0 ALT="{$cluster_url} LOAD"
   SRC="./graph.php?{$graphargs}&amp;g=load_report&amp;z=medium&amp;c={$cluster_url}">
</A>
<A HREF="./graph.php?{$graphargs}&amp;g=mem_report&amp;z=large&amp;c={$cluster_url}">
<IMG BORDER=0 ALT="{$cluster_url} MEM"
   SRC="./graph.php?{$graphargs}&amp;g=mem_report&amp;z=medium&amp;c={$cluster_url}">
</A>
<A HREF="./graph.php?{$graphargs}&amp;g=cpu_report&amp;z=large&amp;c={$cluster_url}">
<IMG BORDER=0 ALT="{$cluster_url} CPU"
   SRC="./graph.php?{$graphargs}&amp;g=cpu_report&amp;z=medium&amp;c={$cluster_url}">
</A>
<A HREF="./graph.php?{$graphargs}&amp;g=network_report&amp;z=large&amp;c={$cluster_url}">
<IMG BORDER=0 ALT="{$cluster_url} NETWORK"
   SRC="./graph.php?{$graphargs}&amp;g=network_report&amp;z=medium&amp;c={$cluster_url}">
</A>
<A HREF="./graph.php?{$graphargs}&amp;g=packet_report&amp;z=large&amp;c={$cluster_url}">
<IMG BORDER=0 ALT="{$cluster_url} PACKETS"
   SRC="./graph.php?{$graphargs}&amp;g=packet_report&amp;z=medium&amp;c={$cluster_url}">
</A>

</TD>
</TR>
</TABLE>
<BR>
<BR>
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
<TD><A HREF="./graph.php?{$g_metric.graphargs}&amp;z=large">
<IMG BORDER=0 ALT="{$g_metric.alt}" SRC="./graph.php?{$g_metric.graphargs}" TITLE="{$g_metric.desc}">
</A></TD>
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
