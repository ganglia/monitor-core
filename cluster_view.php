<?php
/* $Id$ */
$tpl = new TemplatePower( template("cluster_view.tpl") );
$tpl->assignInclude("extra", template("cluster_extra.tpl"));
$tpl->prepare();

$tpl->assign("images","./templates/$template_name/images");

$cpu_num = !$showhosts ? $metrics["cpu_num"][SUM] : cluster_sum("cpu_num", $metrics);
$load_one_sum = !$showhosts ? $metrics["load_one"][SUM] : cluster_sum("load_one", $metrics);
$load_five_sum = !$showhosts ? $metrics["load_five"][SUM] : cluster_sum("load_five", $metrics);
$load_fifteen_sum = !$showhosts ? $metrics["load_fifteen"][SUM] : cluster_sum("load_fifteen", $metrics);
$units = !$showhosts ? $metrics[$metricname][UNITS] : $metrics[key($metrics)][$metricname][UNITS];

$tpl->assign("num_nodes", intval($cluster[HOSTS_UP]));
$tpl->assign("num_dead_nodes", intval($cluster[HOSTS_DOWN]));
$tpl->assign("cpu_num", $cpu_num);
$tpl->assign("localtime", date("Y-m-d H:i", $cluster[LOCALTIME]));

if (!$cpu_num) $cpu_num = 1;
$cluster_load15 = sprintf("%.0f", ((double) $load_fifteen_sum / $cpu_num) * 100);
$cluster_load5 = sprintf("%.0f", ((double) $load_five_sum / $cpu_num) * 100);
$cluster_load1 = sprintf("%.0f", ((double) $load_one_sum / $cpu_num) * 100);
$tpl->assign("cluster_load", "$cluster_load15%, $cluster_load5%, $cluster_load1%");

$cluster_url=rawurlencode($clustername);

$tpl->assign("cluster", $clustername);
#
# Summary graphs
#
$graph_args = "c=$cluster_url&$get_metric_string&st=$cluster[LOCALTIME]";
$tpl->assign("graph_args", $graph_args);
if (!isset($optional_graphs))
	$optional_graphs = array();
foreach ($optional_graphs as $g) {
	$tpl->newBlock('optional_graphs');
	$tpl->assign('name',$g);
	$tpl->assign('graph_args',$graph_args);
	
}

$units=$units ? "($units)" : "";
$tpl->assign("metric","$metricname $units");
$tpl->assign("sort", $sort);
$tpl->assign("range", $range);
# Host columns menu defined in header.php
$tpl->assign("cols_menu", $cols_menu);
$tpl->assign("checked$showhosts", "checked");

$sorted_hosts = array();
$down_hosts = array();
if ($showhosts)
   {
      foreach ($hosts_up as $host => $val)
         {
            $cpus = $metrics[$host]["cpu_num"][VAL];
            if (!$cpus) $cpus=1;
            $load_one  = $metrics[$host]["load_one"][VAL];
            $load = ((float) $load_one)/$cpus;
            $host_load[$host] = $load;
            $percent_hosts[load_color($load)] += 1;
            if ($metricname=="load_one")
               $sorted_hosts[$host] = $load;
            else 
               $sorted_hosts[$host] = $metrics[$host][$metricname][VAL];
         }
         
      foreach ($hosts_down as $host => $val)
         {
            $load = -1.0;
            $down_hosts[$host] = $load;
            $percent_hosts[load_color($load)] += 1;
         }
      
      # Show pie chart of loads
      $pie_args = "title=" . rawurlencode("Cluster Load Percentages");
      $pie_args .= "&size=250x150";
      foreach($load_colors as $name=>$color)
         {
            if (!array_key_exists($color, $percent_hosts))
               continue;
            $n = $percent_hosts[$color];
            $name_url = rawurlencode($name);
            $pie_args .= "&$name_url=$n,$color";
         }
   }
else
   {
      # Show pie chart of hosts up/down
      $pie_args = "title=" . rawurlencode("Host Status");
      $pie_args .= "&size=250x150";
      $up_color = $load_colors["50-75"];
      $down_color = $load_colors["down"];
      $pie_args .= "&Up=$cluster[HOSTS_UP],$up_color";
      $pie_args .= "&Down=$cluster[HOSTS_DOWN],$down_color";
   }
$tpl->assign("pie_args", $pie_args);

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
   case "by hostname":
      ksort($sorted_hosts);
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

      $host_link="\"?c=$cluster_url&h=$host_url&$get_metric_string\"";
      $textval = "";
      #echo "$host: $value, ";

      if ($hosts_down[$host])
         {
            $last_heartbeat = $cluster[LOCALTIME] - $hosts_down[$host][REPORTED];
            $age = $last_heartbeat > 3600 ? uptime($last_heartbeat) : "${last_heartbeat}s";

            $class = "down";
            $textval = "down <br>&nbsp;<font size=-2>Last heartbeat $age ago</font>";
         }
      else
         {
            $val = $metrics[$host][$metricname];
            $class = "metric";

            if ($val[TYPE]=="timestamp" or $always_timestamp[$metricname])
               {
                  $textval = date("r", $val[VAL]);
               }
            elseif ($val[TYPE]=="string" or $val[SLOPE]=="zero" or
               $always_constant[$metricname] or ($max_graphs > 0 and $i > $max_graphs ))
               {
                  $textval = "$val[VAL] $val[UNITS]";
               }
            else
               {
                  $load_color = load_color($host_load[$host]);
                  $graphargs = ($reports[$metricname]) ? "g=$metricname&" : 
                     "m=$metricname&";
                  $graphargs .= "z=small&c=$cluster_url&h=$host_url&l=$load_color"
                     ."&v=$val[VAL]&x=$max&n=$min&r=$range&st=$cluster[LOCALTIME]";
               }
         }

      if ($textval)
         {
            $cell="<td class=$class>".
               "<b><a href=$host_link>$host</a></b><br>".
               "<i>$metricname:</i> <b>$textval</b></td>";
         }
      else
         {
            $cell="<td><a href=$host_link>".
               "<img src=\"./graph.php?$graphargs\" ".
               "alt=\"$host\" height=112 width=225 border=0></a></td>";
         }

      $tpl->assign("metric_image", $cell);
      if (! ($i++ % $hostcols) )
         $tpl->assign ("br", "</tr><tr>");
   }

$tpl->printToScreen();
?>

