<TABLE BORDER="0" WIDTH="100%">

<!-- START BLOCK : source_info -->
<TR>
  <TD CLASS={class} COLSPAN=3>
    <A HREF="{url}"><STRONG>{name}</STRONG></A>{alt_view}
  </TD>
</TR>

<TR>
<!-- START BLOCK : public -->
<TD ALIGN="LEFT" VALIGN="TOP">
<table cellspacing=1 cellpadding=1 width="100%" border=0>
 <tr><td>CPUs Total:</td><td align=left><B>{cpu_num}</B></td></tr>
 <tr><td width="80%">Hosts up:</td><td align=left><B>{num_nodes}</B></td></tr>
 <tr><td>Hosts down:</td><td align=left><B>{num_dead_nodes}</B></td></tr>
 <tr><td>&nbsp;</td></tr>
 <tr><td class=footer colspan=2>{cluster_load}</td></tr>
 <tr><td class=footer colspan=2>{cluster_util}</td></tr>
 <tr><td class=footer colspan=2>{localtime}</td></tr>
</table>
</TD>

<!-- START BLOCK : self_summary_graphs -->
<TD ALIGN="RIGHT" VALIGN="TOP"><TABLE>
  <TR><TD>
    <A HREF="./graph.php?{graph_url}&amp;g=load_report&amp;z=large">
      <IMG SRC="./graph.php?{graph_url}&amp;g=load_report&amp;z=medium"
           ALT="{name} LOAD" BORDER="0">
    </A>
  </TD></TR>
  <TR><TD>
    <A HREF="./graph.php?{graph_url}&amp;g=cpu_report&amp;z=large">
      <IMG SRC="./graph.php?{graph_url}&amp;g=cpu_report&amp;z=medium"
           ALT="{name} CPU" BORDER="0">
    </A>
  </TD></TR>
</TABLE></TD>

<TD VALIGN="TOP"><TABLE>
  <TR><td>
    <A HREF="./graph.php?{graph_url}&amp;g=mem_report&amp;z=large">
      <IMG SRC="./graph.php?{graph_url}&amp;g=mem_report&amp;z=medium"
           ALT="{name} MEM" BORDER="0">
    </A>
  </TD></TR>
  <TR><TD>
    <A HREF="./graph.php?{graph_url}&amp;g=network_report&amp;z=large">
      <IMG SRC="./graph.php?{graph_url}&amp;g=network_report&amp;z=medium"
           ALT="{name} NETWORK" BORDER="0">
    </A>
  </TD></TR>
</TABLE></TD>
 <!-- END BLOCK : self_summary_graphs -->

 <!-- START BLOCK : summary_graphs -->
  <TD ALIGN="RIGHT" VALIGN="TOP">
  <A HREF="{url}">
    <IMG SRC="./graph.php?{graph_url}&amp;g=load_report&amp;z=medium&amp;r={range}"
         ALT="{name} LOAD" BORDER="0">
  </A>
  </TD>

  <TD VALIGN="TOP">
  <A HREF="{url}">
    <IMG SRC="./graph.php?{graph_url}&amp;g=mem_report&amp;z=medium&amp;r={range}"
         ALT="{name} MEM" BORDER="0">
  </A>
  </TD>
 <!-- END BLOCK : summary_graphs -->
<!-- END BLOCK : public -->

<!-- START BLOCK : private -->
  <TD ALIGN="LEFT" VALIGN="TOP">
<table cellspacing=1 cellpadding=1 width="100%" border=0>
 <tr><td>CPUs Total:</td><td align=left><B>{cpu_num}</B></td></tr>
 <tr><td width="80%">Nodes:</td><td align=left><B>{num_nodes}</B></td></tr>
 <tr><td>&nbsp;</td></tr>
 <tr><td class=footer colspan=2>{localtime}</td></tr>
</table>
   </TD>
   <TD COLSPAN=2 align=center>This is a private cluster.</TD>
<!-- END BLOCK : private -->
</TR>
<!-- END BLOCK : source_info -->
</TABLE>

<!-- START BLOCK : show_snapshot -->
<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN="2" CLASS="title">Snapshot of the {self} |
    <FONT SIZE="-1"><A HREF="./cluster_legend.html">Legend</A></FONT>
  </TD>
</TR>
</TABLE>

<CENTER>
<TABLE CELLSPACING=12 CELLPADDING=2>
<!-- START BLOCK : snap_row -->
<TR>{names}</TR>
<TR>{images}</TR>
<!-- END BLOCK : snap_row -->
</TABLE>
</CENTER>
<!-- END BLOCK : show_snapshot -->
