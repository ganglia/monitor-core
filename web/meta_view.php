<?php
/* $Id$ */
$tpl = new TemplatePower( template("meta_view.tpl") );
$tpl->prepare();

# Find the private clusters.  But no one is emabarrassed in the
# control room (public only!). 
if ( $context != "control" ) {
   $private=embarrassed();
}

$source_names = array_keys($grid);

# Build a list of cluster names and randomly pick a smaller subset to
# display for control room mode.  This allows a dedicated host to
# eventually cycle through all the graphs w/o scrolling the mouse.  A bunch
# of these stations could monitor a large grid.
#
# For the standard meta view still display all the hosts.

if ( $context == "control" ) {
       srand((double)microtime()*1000000);
       shuffle($source_names);
       $subset = array_slice($source_names, 0, abs($controlroom));
       $source_names = $subset;
}

foreach( $source_names as $c)
   {
      $cpucount = $metrics[$c]["cpu_num"]['SUM'];
      if (!$cpucount) $cpucount=1;
      $load_one = $metrics[$c]["load_one"]['SUM'];
      $value = (double) $load_one / $cpucount;
      $sorted_sources[$c] = $value;
      isset($total_load) or $total_load = 0;
      $total_load += $value;   
   }
# Insure self is always first 
$sorted_sources[$self] = $total_load + 1;

if ($sort == "descending") {
      arsort($sorted_sources);
}
else if ($sort == "by hostname") {
	  ksort($sorted_sources);
}
else {
      asort($sorted_sources);
}


# Display the sources. The first is ourself, the rest are our children.
foreach ( $sorted_sources as $source => $val )
   {
      $m = $metrics[$source];
      $sourceurl = rawurlencode($source);
      if (isset($grid[$source]['GRID']) and $grid[$source]['GRID'])
         {
            $localtime = $grid[$source]['LOCALTIME'];
            # Is this the our own grid?
            if ($source==$self)
               {
                  # Negative control room values means dont display grid summary.
                  if ($controlroom < 0) continue;
                  $num_sources = count($sorted_sources) - 1;
                  $name = "$self $meta_designator ($num_sources sources)";
                  $graph_url = "me=$sourceurl&amp;$get_metric_string";
                  $url = "./?$get_metric_string";
               }
            else
               {
                  # Set grid context.
                  $name = "$source $meta_designator";
                  $graph_url = "G=$sourceurl&amp;$get_metric_string&amp;st=$localtime";
                  $authority = $grid[$source]['AUTHORITY'];
                  $url = "$authority?gw=fwd&amp;gs=$gridstack_url";
               }
            $alt_url = "<a href=\"./?t=yes&amp;$get_metric_string\">(tree view)</a>";
            $class = "grid";
         }
      else
         {
            # Set cluster context.
            $name = $source;
            $localtime = $grid[$source]['LOCALTIME'];
            $graph_url = "c=$sourceurl&amp;$get_metric_string&amp;st=$localtime";
            $url = "./?c=$sourceurl&amp;$get_metric_string";
            $alt_url = "<a href=\"./?p=2&amp;$graph_url\">(physical view)</a>";
            $class = "cluster";
         }

      $cpu_num = $m["cpu_num"]['SUM'] ? $m["cpu_num"]['SUM'] : 1;
      $cluster_load15 = sprintf("%.0f", ((double) $m["load_fifteen"]['SUM'] / $cpu_num) * 100);
      $cluster_load5 = sprintf("%.0f", ((double) $m["load_five"]['SUM'] / $cpu_num) * 100);
      $cluster_load1 = sprintf("%.0f", ((double) $m["load_one"]['SUM'] / $cpu_num) * 100);
      $cluster_load = "$cluster_load15%, $cluster_load5%, $cluster_load1%";

      $tpl->newBlock ("source_info");
      $tpl->assign("name", $name );
      $tpl->assign("cpu_num", $m["cpu_num"]['SUM']);
      $tpl->assign("url", $url);
      $tpl->assign("class", $class);
      if ($num_sources)
         $tpl->assign("Sources: $num_sources");

      # I dont like this either, but we need to have private clusters because some
      # users are skittish about publishing the load info.
      if (!isset($private[$source]) or !$private[$source]) 
         {
            $tpl->assign("alt_view", "<FONT SIZE=\"-2\">$alt_url</FONT>");
            # Each block has a different namespace, so we need to redefine variables.
            $tpl->newBlock("public");
            if ($localtime)
               $tpl->assign("localtime",  "<font size=-1>Localtime:</font><br>&nbsp;&nbsp;" 
                  . date("Y-m-d H:i", $localtime) );
            if ($cluster_load)
               $tpl->assign("cluster_load", "<font size=-1>Avg Load (15, 5, 1m):</font>"
                  ."<br>&nbsp;&nbsp;$cluster_load");
            $tpl->assign("cpu_num", $m["cpu_num"]['SUM']);
            $tpl->assign("num_nodes", $grid[$source]["HOSTS_UP"] );
            $tpl->assign("num_dead_nodes", $grid[$source]["HOSTS_DOWN"] );
            $tpl->assign("range", $range);
            $tpl->assign("name", $name );
            $tpl->assign("url", $url);
            $tpl->assign("graph_url", $graph_url);
	    if(isset($base64img)) {
                $tpl->assign("base64img", $base64img);
	    }
         }
      else 
         {
            $tpl->newBlock("private");
            $tpl->assign("num_nodes", $grid[$source]["HOSTS_UP"] + $grid[$source]["HOSTS_DOWN"] );
            $tpl->assign("cpu_num", $m["cpu_num"]['SUM']);
            if ($localtime)
               $tpl->assign("localtime", "<font size=-1>Localtime:</font><br>&nbsp;&nbsp;"
                  . date("Y-m-d H:i",$localtime));
         }
   }

# Show load images.
if ($show_meta_snapshot=="yes") {
   $tpl->newBlock("show_snapshot");
   $tpl->assign("self", "$self $meta_designator");

   foreach ($sorted_sources as $c=>$value) {
      if ($c==$self) continue;
      if (isset($private[$c]) and $private[$c]) {
         $Private[$c] = template("images/cluster_private.jpg");
         continue;
      }
      $names[]=$c;

      if (isset($grid[$c]['GRID']) and $grid[$c]['GRID'])
         $image = load_image("grid", $value);
      else
         $image = load_image("cluster", $value);
      $Images[]=$image;
   }
   # Add private cluster pictures to the end.
   if (isset($Private) and is_array($Private)) {
      foreach ($Private as $c=>$image) {
         $names[]=$c;
         $Images[]=$image;
      }
   }

   # All this fancyness is to get the Cluster names
   # above the image. Not easy with template blocks.
   $cols=5;
   $i = 0;
   $count=count($names);
   while ($i < $count)
      {
         $snapnames = "";
         $snapimgs = "";
         $tpl->newBlock("snap_row");
         foreach(range(0, $cols-1) as $j)
            {
               $k = $i + $j;
               if ($k >= $count) break;
               $n = $names[$k];
               $snapnames .= "<td valign=bottom align=center><b>$n</b></td>\n";
               $snapimgs .= "<td valign=top align=center>";
               if (isset($grid[$n]['GRID']) and $grid[$n]['GRID'])
                  $snapimgs .= "<a href=\"" . $grid[$n]['AUTHORITY'] ."?gw=fwd&amp;gs=$gridstack_url\">";
               else
                  {
                     $nameurl = rawurlencode($n);
                     $snapimgs .= "<a href=\"./?c=$nameurl&amp;$get_metric_string\">";
                  }
               $snapimgs .= "<img src=$Images[$k] border=0 align=top></a></td>\n";
            }
         $tpl->assign("names", $snapnames);
         $tpl->assign("images", $snapimgs);
         $i += $cols;
      }
}

$tpl->printToScreen();
?>
