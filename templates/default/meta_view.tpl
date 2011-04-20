{if isset($filters)}
<TABLE BORDER="0" WIDTH="100%">
  <tr>
    {foreach $filters filter}
      <td>
        <b>{$filter.filter_name}</b>
        <SELECT NAME="choose_filter[{$filter.filter_shortname}]" OnChange="ganglia_form.submit();">
          <OPTION NAME=""></OPTION>
          {foreach $filter.choice choice}
          {if $choose_filter.$filter.filter_shortname == $choice}
          <OPTION NAME="{$choice}" selected>{$choice}</OPTION>
          {else}
          <OPTION NAME="{$choice}">{$choice}</OPTION>
          {/if}
          {/foreach}
        </SELECT>
      </TD>
    {/foreach}
  </TR>
</TABLE>
{/if}

<TABLE BORDER="0" WIDTH="100%">

{foreach $sources source}
<TR>
  <TD CLASS={$source.class} COLSPAN=3>
    <A HREF="{$source.url}"><STRONG>{$source.name}</STRONG></A> {$source.alt_view}
  </TD>
</TR>

<TR>
{if isset($source.public)}
<TD ALIGN="LEFT" VALIGN="TOP">
<table cellspacing=1 cellpadding=1 width="100%" border=0>
 <tr><td>CPUs Total:</td><td align=left><B>{$source.cpu_num}</B></td></tr>
 <tr><td width="80%">Hosts up:</td><td align=left><B>{$source.num_nodes}</B></td></tr>
 <tr><td>Hosts down:</td><td align=left><B>{$source.num_dead_nodes}</B></td></tr>
 <tr><td>&nbsp;</td></tr>
 <tr><td class=footer colspan=2>{$source.cluster_load}</td></tr>
 <tr><td class=footer colspan=2>{$source.cluster_util}</td></tr>
 <tr><td class=footer colspan=2>{$source.localtime}</td></tr>
</table>
</TD>

{if isset($source.self_summary_graphs)}
  <TD ALIGN="RIGHT" VALIGN="TOP">
    <A HREF="./graph_all_periods.php?{$source.graph_url}&amp;g=load_report&amp;z=large">
      <IMG SRC="./graph.php?{$source.graph_url}&amp;g=load_report&amp;z=medium"
           ALT="{$source.name} LOAD" BORDER="0">
    </A><BR>
    <A HREF="./graph_all_periods.php?{$source.graph_url}&amp;g=cpu_report&amp;z=large">
      <IMG SRC="./graph.php?{$source.graph_url}&amp;g=cpu_report&amp;z=medium"
           ALT="{$source.name} CPU" BORDER="0">
    </A>
  </TD>
  <TD VALIGN="TOP">
    <A HREF="./graph_all_periods.php?{$source.graph_url}&amp;g=mem_report&amp;z=large">
      <IMG SRC="./graph.php?{$source.graph_url}&amp;g=mem_report&amp;z=medium"
           ALT="{$source.name} MEM" BORDER="0">
    </A><BR>
    <A HREF="./graph_all_periods.php?{$source.graph_url}&amp;g=network_report&amp;z=large">
      <IMG SRC="./graph.php?{$source.graph_url}&amp;g=network_report&amp;z=medium"
           ALT="{$source.name} NETWORK" BORDER="0">
    </A>
  </TD>
{/if}

{if isset($source.summary_graphs)}
  <TD ALIGN="RIGHT" VALIGN="TOP">
  <A HREF="{$source.url}">
    <IMG SRC="./graph.php?{$source.graph_url}&amp;g=load_report&amp;z=medium&amp;r={$source.range}"
         ALT="{$source.name} LOAD" BORDER="0">
  </A>
  </TD>

  <TD VALIGN="TOP">
  <A HREF="{$source.url}">
    <IMG SRC="./graph.php?{$source.graph_url}&amp;g=mem_report&amp;z=medium&amp;r={$source.range}"
         ALT="{$source.name} MEM" BORDER="0">
  </A>
  </TD>
{/if}
{/if}

{if isset($source.private)}
  <TD ALIGN="LEFT" VALIGN="TOP">
<table cellspacing=1 cellpadding=1 width="100%" border=0>
 <tr><td>CPUs Total:</td><td align=left><B>{$source.cpu_num}</B></td></tr>
 <tr><td width="80%">Nodes:</td><td align=left><B>{$source.num_nodes}</B></td></tr>
 <tr><td>&nbsp;</td></tr>
 <tr><td class=footer colspan=2>{$source.localtime}</td></tr>
</table>
   </TD>
   <TD COLSPAN=2 align=center>This is a private cluster.</TD>
{/if}
</TR>
{/foreach}
</TABLE>

{if isset($show_snapshot)}
<TABLE BORDER="0" WIDTH="100%">
<TR>
  <TD COLSPAN="2" CLASS="title">Snapshot of the {$self} |
    <FONT SIZE="-1"><A HREF="./cluster_legend.html">Legend</A></FONT>
  </TD>
</TR>
</TABLE>

<CENTER>
<TABLE CELLSPACING=12 CELLPADDING=2>
{foreach $snap_rows snap_row}
<TR>{$snap_row.names}</TR>
<TR>{$snap_row.images}</TR>
{/foreach}
</TABLE>
</CENTER>
{/if}
