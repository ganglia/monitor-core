<?php
/* $Id$ */
$tpl = new TemplatePower( template("cluster_view.tpl") );
$tpl->assignInclude("extra", template("cluster_extra.tpl"));
$tpl->prepare();

$tpl->assign("images","./templates/$template_name/images");

$cpu_num = !$showhosts ? $metrics["cpu_num"]['SUM'] : cluster_sum("cpu_num", $metrics);
$load_one_sum = !$showhosts ? $metrics["load_one"]['SUM'] : cluster_sum("load_one", $metrics);
$load_five_sum = !$showhosts ? $metrics["load_five"]['SUM'] : cluster_sum("load_five", $metrics);
$load_fifteen_sum = !$showhosts ? $metrics["load_fifteen"]['SUM'] : cluster_sum("load_fifteen", $metrics);
#
# Correct handling of *_report metrics
#
if (!$showhosts) {
  if(array_key_exists($metricname, $metrics))
     $units = $metrics[$metricname]['UNITS'];
  }
else {
  if(array_key_exists($metricname, $metrics[key($metrics)]))
     if (isset($metrics[key($metrics)][$metricname]['UNITS']))
        $units = $metrics[key($metrics)][$metricname]['UNITS'];
     else
        $units = '';
  }

if(isset($cluster['HOSTS_UP'])) {
    $tpl->assign("num_nodes", intval($cluster['HOSTS_UP']));
} else {
    $tpl->assign("num_nodes", 0);
}
if(isset($cluster['HOSTS_DOWN'])) {
    $tpl->assign("num_dead_nodes", intval($cluster['HOSTS_DOWN']));
} else {
    $tpl->assign("num_dead_nodes", 0);
}
$tpl->assign("cpu_num", $cpu_num);
$tpl->assign("localtime", date("Y-m-d H:i", $cluster['LOCALTIME']));

if (!$cpu_num) $cpu_num = 1;
$cluster_load15 = sprintf("%.0f", ((double) $load_fifteen_sum / $cpu_num) * 100);
$cluster_load5 = sprintf("%.0f", ((double) $load_five_sum / $cpu_num) * 100);
$cluster_load1 = sprintf("%.0f", ((double) $load_one_sum / $cpu_num) * 100);
$tpl->assign("cluster_load", "$cluster_load15%, $cluster_load5%, $cluster_load1%");

$avg_cpu_num = find_avg($clustername, "", "cpu_num");
if ($avg_cpu_num == 0) $avg_cpu_num = 1;
$cluster_util = sprintf("%.0f", ((double) find_avg($clustername, "", "load_one") / $avg_cpu_num ) * 100);
$tpl->assign("cluster_util", "$cluster_util%");

$cluster_url=rawurlencode($clustername);


$tpl->assign("cluster", $clustername);
#
# Summary graphs
#
$graph_args = "c=$cluster_url&amp;$get_metric_string&amp;st=$cluster[LOCALTIME]";
$tpl->assign("graph_args", $graph_args);
if (!isset($optional_graphs))
  $optional_graphs = array();
foreach ($optional_graphs as $g) {
  $tpl->newBlock('optional_graphs');
  $tpl->assign('name',$g);
  $tpl->assign('graph_args',$graph_args);
  $tpl->gotoBlock('_ROOT');
}

#
# Correctly handle *_report cases and blank (" ") units
#
if (isset($units)) {
  if ($units == " ")
    $units = "";
  else
    $units=$units ? "($units)" : "";
}
else {
  $units = "";
}
$tpl->assign("metric","$metricname $units");
$tpl->assign("sort", $sort);
$tpl->assign("range", $range);
$tpl->assign("checked$showhosts", "checked");

$sorted_hosts = array();
$down_hosts = array();
$percent_hosts = array();
if ($showhosts)
   {
      foreach ($hosts_up as $host => $val)
         {
            $cpus = $metrics[$host]["cpu_num"]['VAL'];
            if (!$cpus) $cpus=1;
            $load_one  = $metrics[$host]["load_one"]['VAL'];
            $load = ((float) $load_one)/$cpus;
            $host_load[$host] = $load;
            if(isset($percent_hosts[load_color($load)])) { 
                $percent_hosts[load_color($load)] += 1;
            } else {
                $percent_hosts[load_color($load)] = 1;
            }
            if ($metricname=="load_one")
               $sorted_hosts[$host] = $load;
            else if (isset($metrics[$host][$metricname]))
               $sorted_hosts[$host] = $metrics[$host][$metricname]['VAL'];
            else
               $sorted_hosts[$host] = "";
         }
         
      foreach ($hosts_down as $host => $val)
         {
            $load = -1.0;
            $down_hosts[$host] = $load;
            if(isset($percent_hosts[load_color($load)])) {
                $percent_hosts[load_color($load)] += 1;
            } else {
                $percent_hosts[load_color($load)] = 1;
            }
         }
      
      # Show pie chart of loads
      $pie_args = "title=" . rawurlencode("Cluster Load Percentages");
      $pie_args .= "&amp;size=250x150";
      foreach($load_colors as $name=>$color)
         {
            if (!array_key_exists($color, $percent_hosts))
               continue;
            $n = $percent_hosts[$color];
            $name_url = rawurlencode($name);
            $pie_args .= "&$name_url=$n,$color";
         }
      $tpl->assign("pie_args", $pie_args);

      # Host columns menu defined in header.php
      $tpl->newBlock('columns_size_dropdown');
      $tpl->assign("cols_menu", $cols_menu);
      $tpl->assign("size_menu", $size_menu);
      $tpl->newBlock('node_legend');
   }
else
   {
      # Show pie chart of hosts up/down
      $pie_args = "title=" . rawurlencode("Host Status");
      $pie_args .= "&amp;size=250x150";
      $up_color = $load_colors["25-50"];
      $down_color = $load_colors["down"];
      $pie_args .= "&amp;Up=$cluster[HOSTS_UP],$up_color";
      $pie_args .= "&amp;Down=$cluster[HOSTS_DOWN],$down_color";
      $tpl->assign("pie_args", $pie_args);
   }

# No reason to go on if we have no up hosts.
if (!is_array($hosts_up) or !$showhosts) {
   $tpl->printToScreen();
   return;
}

switch ($sort)
{
   case "descending":
      arsort($sorted_hosts);
      break;
   case "by name":
      uksort($sorted_hosts, "strnatcmp");
      break;
   default:
   case "ascending":
      asort($sorted_hosts);
      break;
}

$sorted_hosts = array_merge($down_hosts, $sorted_hosts);

# First pass to find the max value in all graphs for this
# metric. The $start,$end variables comes from get_context.php, 
# included in index.php.
list($min, $max) = find_limits($sorted_hosts, $metricname);

# Second pass to output the graphs or metrics.
$i = 1;

foreach ( $sorted_hosts as $host => $value )
   {
      $tpl->newBlock ("sorted_list");
      $host_url = rawurlencode($host);

      $host_link="\"?c=$cluster_url&amp;h=$host_url&amp;$get_metric_string\"";
      $textval = "";

      #echo "$host: $value, ";

      if (isset($hosts_down[$host]) and $hosts_down[$host])
         {
            $last_heartbeat = $cluster['LOCALTIME'] - $hosts_down[$host]['REPORTED'];
            $age = $last_heartbeat > 3600 ? uptime($last_heartbeat) : "${last_heartbeat}s";

            $class = "down";
            $textval = "down <br>&nbsp;<font size=\"-2\">Last heartbeat $age ago</font>";
         }
      else
         {
            if(isset($metrics[$host][$metricname]))
                $val = $metrics[$host][$metricname];
            else
                $val = NULL;
            $class = "metric";

            if ($val['TYPE']=="timestamp" or 
                (isset($always_timestamp[$metricname]) and
                 $always_timestamp[$metricname]))
               {
                  $textval = date("r", $val['VAL']);
               }
            elseif ($val['TYPE']=="string" or $val['SLOPE']=="zero" or
                    (isset($always_constant[$metricname]) and
                    $always_constant[$metricname] or
                    ($max_graphs > 0 and $i > $max_graphs )))
               {
                  $textval = "$val[VAL]";
                  if (isset($val['UNITS']))
                     $textval .= " $val[UNITS]";
               }
         }

      $size = isset($clustergraphsize) ? $clustergraphsize : 'small';

      if ($hostcols == 0) # enforce small size in multi-host report
         $size = 'small';

      $graphargs = "z=$size&amp;c=$cluster_url&amp;h=$host_url";

      if (isset($host_load[$host])) {
         $load_color = load_color($host_load[$host]);
         $graphargs .= "&amp;l=$load_color&amp;v=$val[VAL]";
      }
      $graphargs .= "&amp;r=$range&amp;su=1&amp;st=$cluster[LOCALTIME]";
      if ($cs)
         $graphargs .= "&amp;cs=" . rawurlencode($cs);
      if ($ce)
         $graphargs .= "&amp;ce=" . rawurlencode($ce);

      if ($showhosts == 1)
         $graphargs .= "&amp;x=$max&amp;n=$min";

      if ($textval)
         {
            $cell="<td class=$class>".
               "<b><a href=$host_link>$host</a></b><br>".
               "<i>$metricname:</i> <b>$textval</b></td>";
         }
      else
         {
            $cell="<td><a href=$host_link><img src=\"./graph.php?";
            $cell .= (isset($reports[$metricname]) and $reports[$metricname])
               ? "g=$metricname" : "m=$metricname";
            $cell .= "&amp;$graphargs\" alt=\"$host\" border=0></a></td>";
         }

      if ($hostcols == 0) {
         $pre = "<td><a href=$host_link><img src=\"./graph.php?g=";
         $post = "&amp;$graphargs\" alt=\"$host\" border=0></a></td>";
         $cell .= $pre . "load_report" . $post;
         $cell .= $pre . "mem_report" . $post;
         $cell .= $pre . "cpu_report" . $post;
         $cell .= $pre . "network_report" . $post;
      }

      $tpl->assign("metric_image", $cell);
      if (! ($i++ % $hostcols) )
         $tpl->assign ("br", "</tr><tr>");
   }

$tpl->printToScreen();
?>

