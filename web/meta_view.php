<?php
$tpl = new Dwoo_Template_File( template("meta_view.tpl") );
$data = new Dwoo_Data();

discover_filters();

if ( !empty($filter_defs) ) {
   $data->assign("filters", $filter_defs);
   $data->assign("choose_filter", $choose_filter);
}

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
      $values[$c] = $value;
      isset($total_load) or $total_load = 0;
      $total_load += $value;   
   }

if ($sort == "descending") {
      $sorted_sources[$self] = 999999999;
      arsort($sorted_sources);
} else if ($sort == "by name") { # SORT HACK to keep $self first; see below:
      $sorted_sources["AAAAA.$self"] = $sorted_sources[$self];
      unset($sorted_sources[$self]);
      ksort($sorted_sources);
} else if ($sort == "by hosts up") {
      foreach ($sorted_sources as $source => $val) {
            $sorted_sources[$source] = intval($grid[$source]['HOSTS_UP']);
      }
      $sorted_sources[$self] = 999999999;
      arsort($sorted_sources);
} else if ($sort == "by hosts down") {
      foreach ($sorted_sources as $source => $val) {
            $sorted_sources[$source] = intval($grid[$source]['HOSTS_DOWN']);
      }
      $sorted_sources[$self] = 999999999;
      arsort($sorted_sources);
} else {
      $sorted_sources[$self] = -1;
      asort($sorted_sources);
}

$sources = array();

# Display the sources. The first is ourself, the rest are our children.
foreach ( $sorted_sources as $source => $val )
   {
      # XXX: SORT HACK to keep $self first; see above
      if ($source == "AAAAA.$self") {
            $source = $self;
      }

      # Make sure the source is permitted by the filters, if any
      if(!filter_permit($source))
         continue;

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
                  $url = "$authority?gw=fwd&amp;gs=$gridstack_url&amp;$get_metric_string";
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

      $clusname = $source == $self ? '' : $source;
      $avg_cpu_num = find_avg($clusname, "", "cpu_num");
      if ($avg_cpu_num == 0) $avg_cpu_num = 1;
      $cluster_util = sprintf("%.0f", ((double) find_avg($clusname, "", "load_one") / $avg_cpu_num ) * 100);

      $sources[$source]["name"] = $name;
      $sources[$source]["cpu_num"] = $m["cpu_num"]['SUM'];
      $sources[$source]["url"] = $url;
      $sources[$source]["class"] = $class;

      # Initialize some variables for the $sources array to be past to the template since private sources
      # may not have these defined
      $sources[$source]["alt_view"] = "";
      $sources[$source]["cluster_load"] = "";
      $sources[$source]["cluster_util"] = "";
      $sources[$source]["num_dead_nodes"] = "";
      $sources[$source]["range"] = "";
      $sources[$source]["graph_url"] = "";
      $sources[$source]["base64img"] = "";

      if ($localtime)
         $sources[$source]["localtime"] = "<font size=\"-1\">Localtime:</font><br>&nbsp;&nbsp;" 
            . date("Y-m-d H:i", $localtime);

      # I dont like this either, but we need to have private clusters because some
      # users are skittish about publishing the load info.
      if (!isset($private[$source]) or !$private[$source]) 
         {
            $sources[$source]["alt_view"] = "<FONT SIZE=\"-2\">$alt_url</FONT>";
            $sources[$source]["public"] = 1;
            if ($cluster_load)
               $sources[$source]["cluster_load"] = "<font size=\"-1\">Current Load Avg (15, 5, 1m):</font>"
                  ."<br>&nbsp;&nbsp;<b>$cluster_load</b>";
            if (isset($cluster_util))
               $sources[$source]["cluster_util"] = "<font size=\"-1\">Avg Utilization (last $range):</font>"
                  ."<br>&nbsp;&nbsp;<b>$cluster_util%</b>";
            $sources[$source]["num_nodes"] = $grid[$source]["HOSTS_UP"];
            $sources[$source]["num_dead_nodes"] = $grid[$source]["HOSTS_DOWN"];
            $sources[$source]["range"] = $range;
            $sources[$source]["graph_url"] = $graph_url;
	    if(isset($base64img)) {
                $sources[$source]["base64img"] = $base64img;
	    }
            if ( $source == $self ) {
               $sources[$source]["self_summary_graphs"] = 1;
            } else {
               $sources[$source]["summary_graphs"] = 1;
            }
         }
      else 
         {
            $sources[$source]["private"] = 1;
            $sources[$source]["num_nodes"] = $grid[$source]["HOSTS_UP"] + $grid[$source]["HOSTS_DOWN"];
         }
   }

$data->assign("sources", $sources);
$snap_rows = array();

# Show load images.
if ($show_meta_snapshot=="yes") {
   $data->assign("show_snapshot", 1);
   $data->assign("self", "$self $meta_designator");

   foreach ($sorted_sources as $c=>$value) {
      if ($c==$self) continue;
      if ($c=="AAAAA.$self") continue;  # SORT HACK; see above
      if (isset($private[$c]) and $private[$c]) {
         $Private[$c] = template("images/cluster_private.jpg");
         continue;
      }
      $names[]=$c;

      if (isset($grid[$c]['GRID']) and $grid[$c]['GRID'])
         $image = load_image("grid", $values[$c]);
      else
         $image = load_image("cluster", $values[$c]);
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
               $snapimgs .= "<img src=\"$Images[$k]\" alt=\"$names[$k]\" border=0 align=top></a></td>\n";
            }
         $snap_rows[$i]["names"] = $snapnames;
         $snap_rows[$i]["images"] = $snapimgs;
         $i += $cols;
      }
   $data->assign("snap_rows", $snap_rows);
}

$dwoo->output($tpl, $data);
?>
