<HTML>
<HEAD>
 <TITLE>{host} user-defined metrics</TITLE>
 <META http-equiv="Content-type" content="text/html; charset=utf-8">
 <META http-equiv="refresh" content="300">
 <LINK rel="stylesheet" href="./styles.css" type="text/css"
</HEAD>
<BODY>

<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN="2" BGCOLOR="#EEEEEE" ALIGN="CENTER">
  <FONT SIZE="+2">{cluster} > {host} </FONT>
  </TD>
</TR>

<TR>
 <TD ALIGN="LEFT" VALIGN="TOP" WIDTH="70%">

<IMG SRC="{node_image}" HEIGHT="60" WIDTH="30" ALT="{host}" BORDER="0">
{node_msg}
<BR>
<BR>
 </TD>
 <TD ALIGN=left>
<a href={host_view}>Back to Host View</a>
 </TD>
</TR>

<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN=5 CLASS=title>User Defined Metrics (gmetrics)</TD>
</TR>
<TR>
<TH>TN</TH>
<TH>TMAX</TH>
<TH>DMAX</TH>
<TH>NAME</TH>
<TH>VALUE</TH>

<!-- START BLOCK : g_metric_info -->
<TR>
 <TD CLASS=footer>{tn}</TD><TD CLASS=footer>{tmax}</TD><TD CLASS=footer>{dmax}</TD>
 <TD>{name}</TD><TD>{value}</TD>
</TR>
<!-- END BLOCK : g_metric_info -->
</TABLE>

 </TD>
</TR>
</TABLE>

</BODY>
</HTML>
