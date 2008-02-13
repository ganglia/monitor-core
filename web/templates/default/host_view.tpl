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
  <FONT SIZE="+2">{host} Overview</FONT>
  </TD>
</TR>

<TR>
 <TD ALIGN="LEFT" VALIGN="TOP">

<IMG SRC="{node_image}" HEIGHT="60" WIDTH="30" ALT="{host}" BORDER="0">
{node_msg}
<P>

<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN="2" CLASS=title>Time and String Metrics</TD>
</TR>

<!-- START BLOCK : string_metric_info -->
<TR>
 <TD CLASS=footer WIDTH=30%>{name}</TD><TD>{value}</TD>
</TR>
<!-- END BLOCK : string_metric_info -->

<TR><TD>&nbsp;</TD></TR>

<TR>
  <TD COLSPAN=2 CLASS=title>Constant Metrics</TD>
</TR>

<!-- START BLOCK : const_metric_info -->
<TR>
 <TD CLASS=footer WIDTH=30%>{name}</TD><TD>{value}</TD>
</TR>
<!-- END BLOCK : const_metric_info -->

<TR><TD>&nbsp;</TD></TR>

<TR>
 <TD COLSPAN=2 CLASS=title>
 <a href="host_gmetrics.php?c={cluster_url}&h={host}">Gmetrics</a>
 </TD>
</TR>
</TABLE>

 <HR>
<!-- INCLUDE BLOCK : extra -->

</TD>

<TD ALIGN="CENTER" VALIGN="TOP" WIDTH="395">
<A HREF="./graph.php?g=load_report&z=large&c={cluster_url}&{graphargs}">
<IMG BORDER=0 ALT="{cluster_url} LOAD"
   SRC="./graph.php?g=load_report&z=medium&c={cluster_url}&{graphargs}">
</A>
<A HREF="./graph.php?g=mem_report&z=large&c={cluster_url}&{graphargs}">
<IMG BORDER=0 ALT="{cluster_url} MEM"
   SRC="./graph.php?g=mem_report&z=medium&c={cluster_url}&{graphargs}">
</A>
<A HREF="./graph.php?g=cpu_report&z=large&c={cluster_url}&{graphargs}">
<IMG BORDER=0 ALT="{cluster_url} CPU"
   SRC="./graph.php?g=cpu_report&z=medium&c={cluster_url}&{graphargs}">
</A>
<A HREF="./graph.php?g=network_report&z=large&c={cluster_url}&{graphargs}">
<IMG BORDER=0 ALT="{cluster_url} NETWORK"
   SRC="./graph.php?g=network_report&z=medium&c={cluster_url}&{graphargs}">
</A>
<A HREF="./graph.php?g=packet_report&z=large&c={cluster_url}&{graphargs}">
<IMG BORDER=0 ALT="{cluster_url} PACKETS"
   SRC="./graph.php?g=packet_report&z=medium&c={cluster_url}&{graphargs}">
</A>

</TD>
</TR>
</TABLE>
<BR>
<BR>
<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD CLASS=title>
  {host} <strong>graphs</strong>
  last <strong>{range}</strong>
  sorted <strong>{sort}</strong>
  </TD>
</TR>
</TABLE>

<CENTER>
<TABLE>
<TR>
 <TD>

<!-- START BLOCK : vol_group_info -->
<A HREF="javascript:toggleLayer('{group}');" TITLE="Toggle {group} metrics group on/off">
<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD CLASS=metric>
  {group} metrics
  </TD>
</TR>
</TABLE>
</A>
<DIV ID="{group}">
<!-- START BLOCK : vol_metric_info -->
<A HREF="./graph.php?&{graphargs}&z=large">
<IMG BORDER=0 ALT="{alt}" SRC="./graph.php?{graphargs}" TITLE="{desc}">{br}
</A>
<!-- END BLOCK : vol_metric_info -->
</DIV>
<!-- END BLOCK : vol_group_info -->

 </TD>
</TR>
</TABLE>
</CENTER>
