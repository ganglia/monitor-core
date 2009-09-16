<?php
/* $Id$ */

# Check if this context is private.
include_once "./auth.php";
include_once "./calendar.php";
checkcontrol();
checkprivate();

# RFM - These definitions are here to eliminate "undefined variable"
# error messages in ssl_error_log.
!isset($initgrid) and $initgrid = 0;
!isset($metricname) and $metricname = "";
!isset($context_metrics) and $context_metrics = "";

if ( $context == "control" && $controlroom < 0 )
      $header = "header-nobanner";
else
      $header = "header";

#
# sacerdoti: beginning of Grid tree state handling
#
$me = $self . "@";
array_key_exists($self, $grid) and $me = $me . $grid[$self]['AUTHORITY'];
if ($initgrid)
   {
      $gridstack = array();
      $gridstack[] = $me;
   }
else if ($gridwalk=="fwd")
   {
      # push our info on gridstack, format is "name@url>name2@url".
      if (end($gridstack) != $me)
         {
            $gridstack[] = $me;
         }
   }
else if ($gridwalk=="back")
   {
      # pop a single grid off stack.
      if (end($gridstack) != $me)
         {
            array_pop($gridstack);
         }
   }
$gridstack_str = join(">", $gridstack);
$gridstack_url = rawurlencode($gridstack_str);

if (strstr($clustername, "http://")) {
   header("Location: $clustername?gw=fwd&amp;gs=$gridstack_url");
}

if ($initgrid or $gridwalk)
   {
      # Use cookie so we dont have to pass gridstack around within this site.
      # Cookie values are automatically urlencoded. Expires in a day.
      if ( !isset($_COOKIE["gs"]) or $_COOKIE["gs"] != $gridstack_str )
            setcookie("gs", $gridstack_str, time() + 86400);
   }

# Invariant: back pointer is second-to-last element of gridstack. Grid stack
# never has duplicate entries.
# RFM - The original line caused an error when count($gridstack) = 1.  This
# should fix that.
$parentgrid = $parentlink = NULL;
if(count($gridstack) > 1) {
  list($parentgrid, $parentlink) = explode("@", $gridstack[count($gridstack)-2]);
}

$tpl = new TemplatePower( template("$header.tpl") );
$tpl->prepare();
$tpl->assign("page_title", $title);
$tpl->assign("refresh", $default_refresh);

# Templated Logo image
$tpl->assign("images","./templates/$template_name/images");

$tpl->assign( "date", date("r"));

# The page to go to when "Get Fresh Data" is pressed.
if (isset($page))
      $tpl->assign("page",$page);
else
      $tpl->assign("page","./");

#
# Used when making graphs via graph.php. Included in most URLs
#
$sort_url=rawurlencode($sort);
$get_metric_string = "m=$metricname&amp;r=$range&amp;s=$sort_url&amp;hc=$hostcols&amp;mc=$metriccols";
if ($jobrange and $jobstart)
    $get_metric_string .= "&amp;jr=$jobrange&amp;js=$jobstart";
if ($cs)
    $get_metric_string .= "&amp;cs=" . rawurlencode($cs);
if ($ce)
    $get_metric_string .= "&amp;ce=" . rawurlencode($ce);

# Set the Alternate view link.
$cluster_url=rawurlencode($clustername);
$node_url=rawurlencode($hostname);

# Make some information available to templates.
$tpl->assign("cluster_url", $cluster_url);

if ($context=="cluster")
   {
      $tpl->assign("alt_view", "<a href=\"./?p=2&amp;c=$cluster_url\">Physical View</a>");
   }
elseif ($context=="physical")
   {
      $tpl->assign("alt_view", "<a href=\"./?c=$cluster_url\">Full View</a>");
   }
elseif ($context=="node")
   {
      $tpl->assign("alt_view",
      "<a href=\"./?c=$cluster_url&amp;h=$node_url&amp;$get_metric_string\">Host View</a>");
   }
elseif ($context=="host")
   {
      $tpl->assign("alt_view",
      "<a href=\"./?p=2&amp;c=$cluster_url&amp;h=$node_url\">Node View</a>");
   }

# Build the node_menu
$node_menu = "";

if ($parentgrid) 
   {
      $node_menu .= "<B><A HREF=\"$parentlink?gw=back&amp;gs=$gridstack_url&amp;$get_metric_string\">".
         "$parentgrid $meta_designator</A></B> ";
      $node_menu .= "<B>&gt;</B>\n";
   }

# Show grid.
$mygrid =  ($self == "unspecified") ? "" : $self;
$node_menu .= "<B><A HREF=\"./?$get_metric_string\">$mygrid $meta_designator</A></B> ";
$node_menu .= "<B>&gt;</B>\n";

if ($physical)
   $node_menu .= hiddenvar("p", $physical);

if ( $clustername )
   {
      $url = rawurlencode($clustername);
      $node_menu .= "<B><A HREF=\"./?c=$url&amp;$get_metric_string\">$clustername</A></B> ";
      $node_menu .= "<B>&gt;</B>\n";
      $node_menu .= hiddenvar("c", $clustername);
   }
else
   {
      # No cluster has been specified, so drop in a list
      $node_menu .= "<SELECT NAME=\"c\" OnChange=\"ganglia_form.submit();\">\n";
      $node_menu .= "<OPTION VALUE=\"\">--Choose a Source\n";
      ksort($grid);
      foreach( $grid as $k => $v )
         {
            if ($k==$self) continue;
            if (isset($v['GRID']) and $v['GRID'])
               {
                  $url = $v['AUTHORITY'];
                  $node_menu .="<OPTION VALUE=\"$url\">$k $meta_designator\n";
               }
            else
               {
                  $url = rawurlencode($k);
                  $node_menu .="<OPTION VALUE=\"$url\">$k\n";
               }
         }
      $node_menu .= "</SELECT>\n";
   }

if ( $clustername && !$hostname )
   {
      # Drop in a host list if we have hosts
      if (!$showhosts) {
            $node_menu .= "[Summary Only]";
      }
      elseif (is_array($hosts_up) || is_array($hosts_down))
         {
            $node_menu .= "<SELECT NAME=\"h\" OnChange=\"ganglia_form.submit();\">";
            $node_menu .= "<OPTION VALUE=\"\">--Choose a Node\n";
            if(is_array($hosts_up))
               {
                  uksort($hosts_up, "strnatcmp");
                  foreach($hosts_up as $k=> $v)
                     {
                        $url = rawurlencode($k);
                        $node_menu .= "<OPTION VALUE=\"$url\">$k\n";
                     }
               }
            if(is_array($hosts_down))
               {
                  uksort($hosts_down, "strnatcmp");
                  foreach($hosts_down as $k=> $v)
                     {
                        $url = rawurlencode($k);
                        $node_menu .= "<OPTION VALUE=\"$url\">$k\n";
                     }
               }
            $node_menu .= "</SELECT>\n";
         }
      else
         {
            $node_menu .= "<B>No Hosts</B>\n";
         }
   }
else
   {
      $node_menu .= "<B>$hostname</B>\n";
      $node_menu .= hiddenvar("h", $hostname);
   }

# Save other CGI variables
$node_menu .= hiddenvar("cr", $controlroom);
$node_menu .= hiddenvar("js", $jobstart);
$node_menu .= hiddenvar("jr", $jobrange);

$tpl->assign("node_menu", $node_menu);


//////////////////// Build the metric menu ////////////////////////////////////

if( $context == "cluster" )
   {
   if (!count($metrics)) {
      echo "<h4>Cannot find any metrics for selected cluster \"$clustername\", exiting.</h4>\n";
      echo "Check ganglia XML tree (telnet $ganglia_ip $ganglia_port)\n";
      exit;
   }
   $firsthost = key($metrics);
   foreach ($metrics[$firsthost] as $m => $foo)
         $context_metrics[] = $m;
   foreach ($reports as $r => $foo)
         $context_metrics[] = $r;
   }

#
# If there are graphs present, show ranges.
#
if (!$physical) {
   $context_ranges = array_keys( $time_ranges );
   if ($jobrange)
      $context_ranges[]="job";
   if ($cs or $ce)
      $context_ranges[]="custom";

   $range_menu = "<B>Last</B>&nbsp;&nbsp;"
      ."<SELECT NAME=\"r\" OnChange=\"ganglia_submit();\">\n";
   foreach ($context_ranges as $v) {
      $url=rawurlencode($v);
      $range_menu .= "<OPTION VALUE=\"$url\"";
      if ($v == $range)
         $range_menu .= "SELECTED";
      $range_menu .= ">$v\n";
   }
   $range_menu .= "</SELECT>\n";

   $tpl->assign("range_menu", $range_menu);
}

#
# Only show metric list if we have some and are in cluster context.
#
if (is_array($context_metrics) and $context == "cluster")
   {
      $metric_menu = "<B>Metric</B>&nbsp;&nbsp;"
         ."<SELECT NAME=\"m\" OnChange=\"ganglia_form.submit();\">\n";

      sort($context_metrics);
      foreach( $context_metrics as $k )
         {
            $url = rawurlencode($k);
            $metric_menu .= "<OPTION VALUE=\"$url\" ";
            if ($k == $metricname )
                  $metric_menu .= "SELECTED";
            $metric_menu .= ">$k\n";
         }
      $metric_menu .= "</SELECT>\n";

      $tpl->assign("metric_menu", $metric_menu );      
   }


#
# Show sort order if there is more than one physical machine present.
#
if ($context == "meta" or $context == "cluster")
   {
      $context_sorts[]="ascending";
      $context_sorts[]="descending";
      $context_sorts[]="by name";

      #
      # Show sort order options for meta context only:
      #
      if ($context == "meta" ) {
          $context_sorts[]="by hosts up";
          $context_sorts[]="by hosts down";
      }

      $sort_menu = "<B>Sorted</B>&nbsp;&nbsp;"
         ."<SELECT NAME=\"s\" OnChange=\"ganglia_form.submit();\">\n";
      foreach ( $context_sorts as $v )
         {
            $url = rawurlencode($v);
            $sort_menu .= "<OPTION VALUE=\"$url\" ";
            if ($v == $sort )
                  $sort_menu .= "SELECTED";

            $sort_menu .= ">$v\n";
         }
      $sort_menu .= "</SELECT>\n";

      $tpl->assign("sort_menu", $sort_menu );
   }
   
if ($context == "physical" or $context == "cluster" or $context == 'host' )
   {
      # Present a width list
      $cols_menu = "<SELECT NAME=\"hc\" OnChange=\"ganglia_form.submit();\">\n";

      foreach(range(0,25) as $cols)
         {
            $cols_menu .= "<OPTION VALUE=$cols ";
            if ($cols == $hostcols)
               $cols_menu .= "SELECTED";
            $cols_menu .= ">$cols\n";
         }
      $cols_menu .= "</SELECT>\n";
      
      $size_menu = '<SELECT NAME="z" OnChange="ganglia_form.submit();">';
      
      $size_arr = $graph_sizes_keys;
      foreach ($size_arr as $size) {
          if ($size == "default")
              continue;
          $size_menu .= "<OPTION VALUE=\"$size\"";
          if (    ( isset($clustergraphsize) && ($size === $clustergraphsize)) 
               || (!isset($clustergraphsize) && ($size === 'medium' ))) {
              $size_menu .= " SELECTED";
          }
          $size_menu .= ">$size</OPTION>\n";
      }
      $size_menu .= "</SELECT>\n";
  
      # Assign template variable in cluster view.
   }
if ($context == "host")
   {
      # Present a width list
      $metric_cols_menu = "<SELECT NAME=\"mc\" OnChange=\"ganglia_form.submit();\">\n";

      foreach(range(1,25) as $metric_cols)
         {
            $metric_cols_menu .= "<OPTION VALUE=$metric_cols ";
            if ($metric_cols == $metriccols)
               $metric_cols_menu .= "SELECTED";
            $metric_cols_menu .= ">$metric_cols\n";
         }
      $metric_cols_menu .= "</SELECT>\n";
   }
if ($context == "meta" or $context == "cluster" or $context == "host")
   {
      $examples = "Feb 27 2007 00:00, 2/27/2007, 27.2.2007, now -1 week,"
         . " -2 days, start + 1 hour, etc.";
      $custom_time = "or from <INPUT TYPE=\"TEXT\" TITLE=\"$examples\" NAME=\"cs\" ID=\"cs\" SIZE=\"17\"";
      if ($cs)
         $custom_time .= " value=\"$cs\"";
      $custom_time .= "> to <INPUT TYPE=\"TEXT\" TITLE=\"$examples\" NAME=\"ce\" ID=\"ce\" SIZE=\"17\"";
      if ($ce)
         $custom_time .= " value=\"$ce\"";
      $custom_time .= "><input type=\"submit\" value=\"Go\">\n";
      $custom_time .= "<input type=\"button\" value=\"Clear\" onclick=\"ganglia_submit(1)\">\n";
      $custom_time .= $calendar;
      $tpl->assign("custom_time", $custom_time);

      $tpl->assign("custom_time_head", $calendar_head);
   }

# Make sure that no data is cached..
header ("Expires: Mon, 26 Jul 1997 05:00:00 GMT");    # Date in the past
header ("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT"); # always modified
header ("Cache-Control: no-cache, must-revalidate");  # HTTP/1.1
header ("Pragma: no-cache");                          # HTTP/1.0

$tpl->printToScreen();
?>
