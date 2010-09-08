<?php
/* $Id$ */
$tpl = new Dwoo_Template_File( template("cluster_view.tpl") );
$data = new Dwoo_Data();
$data->assign("extra", template("cluster_extra.tpl"));

$data->assign("images","./templates/$template_name/images");

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
    $data->assign("num_nodes", intval($cluster['HOSTS_UP']));
} else {
    $data->assign("num_nodes", 0);
}
if(isset($cluster['HOSTS_DOWN'])) {
    $data->assign("num_dead_nodes", intval($cluster['HOSTS_DOWN']));
} else {
    $data->assign("num_dead_nodes", 0);
}
$data->assign("cpu_num", $cpu_num);
$data->assign("localtime", date("Y-m-d H:i", $cluster['LOCALTIME']));

if (!$cpu_num) $cpu_num = 1;
$cluster_load15 = sprintf("%.0f", ((double) $load_fifteen_sum / $cpu_num) * 100);
$cluster_load5 = sprintf("%.0f", ((double) $load_five_sum / $cpu_num) * 100);
$cluster_load1 = sprintf("%.0f", ((double) $load_one_sum / $cpu_num) * 100);
$data->assign("cluster_load", "$cluster_load15%, $cluster_load5%, $cluster_load1%");

$avg_cpu_num = find_avg($clustername, "", "cpu_num");
if ($avg_cpu_num == 0) $avg_cpu_num = 1;
$cluster_util = sprintf("%.0f", ((double) find_avg($clustername, "", "load_one") / $avg_cpu_num ) * 100);
$data->assign("cluster_util", "$cluster_util%");

$cluster_url=rawurlencode($clustername);


$data->assign("cluster", $clustername);
#
# Summary graphs
#
$graph_args = "c=$cluster_url&amp;$get_metric_string&amp;st=$cluster[LOCALTIME]";
$data->assign("graph_args", $graph_args);
if (!isset($optional_graphs))
  $optional_graphs = array();
$optional_graphs_data = array();
foreach ($optional_graphs as $g) {
  $optional_graphs_data[$g]['name'] = $g;
  $optional_graphs_data[$g]['graph_args'] = $graph_args;
}
$data->assign('optional_graphs_data', $optional_graphs_data);

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
$data->assign("metric","$metricname $units");
$data->assign("sort", $sort);
$data->assign("range", $range);

$showhosts_levels = array(
   2 => array('checked'=>'', 'name'=>'Auto'),
   1 => array('checked'=>'', 'name'=>'Same'),
   0 => array('checked'=>'', 'name'=>'None'),
);
$showhosts_levels[$showhosts]['checked'] = 'checked';
$data->assign("showhosts_levels", $showhosts_levels);

$sorted_hosts = array();
$down_hosts = array();
$percent_hosts = array();
if ($showhosts)
   {
      foreach ($hosts_up as $host => $val)
         {
               if ( isset($metrics[$host]["cpu_num"]['VAL']) and $metrics[$host]["cpu_num"]['VAL'] != 0 ){
               $cpus = $metrics[$host]["cpu_num"]['VAL'];
            } else {
               $cpus = 1;
            }
            if ( isset($metrics[$host]["load_one"]['VAL']) ){
               $load_one = $metrics[$host]["load_one"]['VAL'];
            } else {
               $load_one = 0;
            }
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
      $data->assign("pie_args", $pie_args);

      # Host columns menu defined in header.php
      $data->assign("columns_size_dropdown", 1);
      $data->assign("cols_menu", $cols_menu);
      $data->assign("size_menu", $size_menu);
      $data->assign("node_legend", 1);
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
      $data->assign("pie_args", $pie_args);
   }

# No reason to go on if we have no up hosts.
if (!is_array($hosts_up) or !$showhosts) {
   $dwoo->output($tpl, $data);
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

      $sorted_list[$host]["metric_image"] = $cell;
      if (! ($i++ % $hostcols) ) {
         $sorted_list[$host]["br"] = "</tr><tr>";
      } else {
         $sorted_list[$host]["br"] = "";
      }
   }

$data->assign("sorted_list", $sorted_list);
$dwoo->output($tpl, $data);
?>
