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
<IMG HEIGHT="147" WIDTH="395" ALT="{cluster_url} LOAD"
   SRC="./graph.php?g=load_report&z=medium&c={cluster_url}&{graphargs}">
<IMG HEIGHT="160" WIDTH="395" ALT="{cluster_url} MEM"
   SRC="./graph.php?g=mem_report&z=medium&c={cluster_url}&{graphargs}">
<IMG HEIGHT="147" WIDTH="395" ALT="{cluster_url} CPU"
   SRC="./graph.php?g=cpu_report&z=medium&c={cluster_url}&{graphargs}">
<IMG HEIGHT="147" WIDTH="395" ALT="{cluster_url} NETWORK"
   SRC="./graph.php?g=network_report&z=medium&c={cluster_url}&{graphargs}">
<IMG HEIGHT="147" WIDTH="395" ALT="{cluster_url} PACKETS"
   SRC="./graph.php?g=packet_report&z=medium&c={cluster_url}&{graphargs}">

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

<!-- START BLOCK : vol_metric_info -->
<IMG HEIGHT="147" WIDTH="395" ALT="{alt}" SRC="./graph.php?{graphargs}">{br}
<!-- END BLOCK : vol_metric_info -->

 </TD>
</TR>
</TABLE>
</CENTER>
