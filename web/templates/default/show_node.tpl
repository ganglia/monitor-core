<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN="2" CLASS=title>{name} Info</TD>
  </TD>
</TR>
<TR><TD COLSPAN="1">&nbsp;</TD></TR>
<TR>
<TD ALIGN="CENTER">

<table cellpadding=2 cellspacing=7 border=1>
<tr>
<td class={class}>
   <table cellpadding=1 cellspacing=10 border=0>
   <tr><td valign=top><font size="+2"><b>{name}</b></font><br>
   <i>{ip}</i><br>
   <em>Location:</em> {location}<p>

   Cluster local time {clustertime}<br>
   Last heartbeat received {age} ago.<br>
   Uptime {uptime}<br>

   </td>
   <td align=right valign=top>
   <table cellspacing=4 cellpadding=3 border=0>
   <tr><td><i>Load:</i></td>
   <td class={load1}><small>{load_one}</small></td>
   <td class={load5}><small>{load_five}</small></td>
   <td class={load15}><small>{load_fifteen}</small></td>
   </tr><em><tr><td></td><td>1m</td><td>5m</td><td>15m</td></tr></em>
   </tr></table><br>

   <table cellspacing=4 cellpadding=3 border=0><tr>
   <td><i>CPU Utilization:</i></td>
   <td class={user}><small>{cpu_user}</small></td>
   <td class={sys}><small>{cpu_system}</small></td>
   <td class={idle}><small>{cpu_idle}</small></td>
   </tr><em><tr><td></td><td>user</td><td>sys</td><td>idle</td></tr></em>
   </tr></table>
   </td>
   </tr>
   <tr><td align=left valign=top>

   <b>Hardware</b><br>
   <em>CPU{s}:</em> {cpu}<br>
   <em>Memory (RAM):</em> {mem}<br>
   <em>Local Disk:</em> {disk}<br>
   <em>Most Full Disk Partition:</em> {part_max_used}
   </td>
   <td align=left valign=top>

   <b>Software</b><br>
   <em>OS:</em> {OS}<br>
   <em>Booted:</em> {booted}<br>
   <em>Uptime:</em> {uptime}<br>
   <em>Swap:</em> {swap}<br>

   </td></tr></table>
</td>
</tr></table>

 </TD>
</TR>
<TR>
<TD align=center valign=center>
 <a href="{physical_view}">Physical View</a> | 
 <a href="{self}">Reload</a>
 <p>
</TD>
</TR>
<TR>
 <TD>
<!-- INCLUDE BLOCK : extra -->
 </TD>
</TR>
</TABLE>
