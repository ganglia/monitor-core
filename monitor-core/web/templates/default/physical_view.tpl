<TABLE CELLPADDING=1 BORDER="0" WIDTH="100%">
<TR>
<TD COLSPAN=3 CLASS=title>{$cluster} cluster - Physical View |
 <FONT SIZE="-1">Columns&nbsp;&nbsp;{$cols_menu}</FONT>
</TD>

<TR>
<TD align=center valign=top>
   Verbosity level (Lower is more compact):<br>
   {foreach $verbosity_levels verbosity checked}
   {$verbosity} <INPUT type=radio name="p" value="{$verbosity}" OnClick="ganglia_form.submit();" {$checked}>&nbsp;
   {/foreach}
</TD>

<TD align=left valign=top width="25%">
Total CPUs: <b>{$CPUs}</b><br>
Total Memory: <b>{$Memory}</b><br>
</TD>

<TD align=left valign=top width="25%">
Total Disk: <b>{$Disk}</b><br>
Most Full Disk: <b>{$most_full}</b><br>
</TD>

</TR>
<TR>
<TD ALIGN="LEFT" COLSPAN=3>

<table cellspacing=20>
<tr>
{foreach $racks rack}
   <td valign=top align=center>
      <table cellspacing=5 border=0>
         {$rack.RackID}
         {$rack.nodes}
      </table>
   </td>
   {$rack.tr}
{/foreach}
</tr></table>

</TD></TR>
</TABLE>

<hr>


<table border=0>
<tr>
 <td align=left>
<font size="-1">
Legend
</font>
 </td>
</tr>
<tr>
<td class=odd>
<table width="100%" cellpadding=1 cellspacing=0 border=0>
<tr>
 <td style="color: blue">Node Name&nbsp;<br></td>
 <td align=right valign=top>
  <table cellspacing=1 cellpadding=3 border=0>
  <tr>
  <td class=L1 align=right>
  <font size="-1">1-min load</font>
  </td>
  </tr>
 </table>
<tr>
<td colspan=2 style="color: rgb(70,70,70)">
<font size="-1">
<em>cpu: </em> CPU clock (GHz) (num CPUs)<br>
<em>mem: </em> Total Memory (GB)
</font>
</td>
</tr>
</table>

</td>
</tr>
</table>
