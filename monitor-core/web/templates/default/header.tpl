<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<HTML>
<HEAD>
<TITLE>Ganglia:: {page_title}</TITLE>
<META http-equiv="Content-type" content="text/html; charset=utf-8">
<META http-equiv="refresh" content="{refresh}{redirect}" >
<LINK rel="stylesheet" href="./styles.css" type="text/css">
</HEAD>
<BODY BGCOLOR="#FFFFFF">

<FORM ACTION="{page}" METHOD="GET" NAME="ganglia_form">
<TABLE WIDTH="100%">
<TR>
  <TD ROWSPAN="2" WIDTH="150">
  <A HREF="http://ganglia.sourceforge.net/">
  <IMG SRC="{images}/logo.jpg" HEIGHT="118" WIDTH="150" 
      ALT="Ganglia" BORDER="0"></A>
  </TD>
  <TD VALIGN="TOP">

  <TABLE WIDTH="100%" CELLPADDING="8" CELLSPACING="0" BORDER=0>
  <TR BGCOLOR="#DDDDDD">
     <TD BGCOLOR="#DDDDDD">
     <FONT SIZE="+1">
     <B>{page_title} for {date}</B>
     </FONT>
     </TD>
     <TD BGCOLOR="#DDDDDD" ALIGN="RIGHT">
     <INPUT TYPE="SUBMIT" VALUE="Get Fresh Data">
     </TD>
  </TR>
  <TR>
     <TD COLSPAN="1">
     {metric_menu} &nbsp;&nbsp;
     {range_menu}&nbsp;&nbsp;
     {sort_menu}
     </TD>
     <TD>
      <B>{alt_view}</B>
     </TD>
  </TR>
  <tr>
     <td colspan="1">
       <FONT SIZE="+1">
         {node_menu}
       </FONT>
     </td>
  </tr> 
  </TABLE>

  </TD>
</TR>
</TABLE> 
<HR SIZE="1" NOSHADE>
