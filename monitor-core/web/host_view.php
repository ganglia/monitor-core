<?php
/* $Id$ */

$tpl = new Dwoo_Template_File( template("host_view.tpl") );
$data = new Dwoo_Data();
$data->assign("extra", template("host_extra.tpl"));

$data->assign("cluster", $clustername);
$data->assign("host", $hostname);
$data->assign("node_image", node_image($metrics));
$data->assign("sort",$sort);
$data->assign("range",$range);

if($hosts_up)
      $data->assign("node_msg", "This host is up and running."); 
else
      $data->assign("node_msg", "This host is down."); 

$cluster_url=rawurlencode($clustername);
$data->assign("cluster_url", $cluster_url);
$data->assign("graphargs", "h=$hostname&amp;$get_metric_string&amp;st=$cluster[LOCALTIME]");

# For the node view link.
$data->assign("node_view","./?p=2&amp;c=$cluster_url&amp;h=$hostname");

# No reason to go on if this node is down.
if ($hosts_down)
   {
      $dwoo->output($tpl, $data);
      return;
   }

$data->assign("ip", $hosts_up['IP']);
$data->assign('columns_dropdown', 1);
$data->assign("metric_cols_menu", $metric_cols_menu);
$data->assign("size_menu", $size_menu);
$g_metrics_group = array();

foreach ($metrics as $name => $v)
   {
       if ($v['TYPE'] == "string" or $v['TYPE']=="timestamp" or
           (isset($always_timestamp[$name]) and $always_timestamp[$name]))
          {
             $s_metrics[$name] = $v;
          }
       elseif ($v['SLOPE'] == "zero" or
               (isset($always_constant[$name]) and $always_constant[$name]))
          {
             $c_metrics[$name] = $v;
          }
       else if (isset($reports[$name]) and $reports[$metric])
          continue;
       else
          {
             $size = isset($clustergraphsize) ? $clustergraphsize : 'default';
             $size = $size == 'medium' ? 'default' : $size; //set to 'default' to preserve old behavior
             $graphargs = "c=$cluster_url&amp;h=$hostname&amp;v=$v[VAL]"
               ."&amp;m=$name&amp;r=$range&amp;z=$size&amp;jr=$jobrange"
               ."&amp;js=$jobstart&amp;st=$cluster[LOCALTIME]";
             if ($cs)
                $graphargs .= "&amp;cs=" . rawurlencode($cs);
             if ($ce)
                $graphargs .= "&amp;ce=" . rawurlencode($ce);
             # Adding units to graph 2003 by Jason Smith <smithj4@bnl.gov>.
             if ($v['UNITS']) {
                $encodeUnits = rawurlencode($v['UNITS']);
                $graphargs .= "&amp;vl=$encodeUnits";
             }
             if (isset($v['TITLE'])) {
                $title = $v['TITLE'];
                $graphargs .= "&amp;ti=$title";
             }
             $g_metrics[$name]['graph'] = $graphargs;
             $g_metrics[$name]['description'] = isset($v['DESC']) ? $v['DESC'] : '';

             # Setup an array of groups that can be used for sorting in group view
             if ( isset($metrics[$name]['GROUP']) ) {
                $groups = $metrics[$name]['GROUP'];
             } else {
                $groups = array("");
             }

             foreach ( $groups as $group) {
                if ( isset($g_metrics_group[$group]) ) {
                   $g_metrics_group[$group] = array_merge($g_metrics_group[$group], (array)$name);
                } else {
                   $g_metrics_group[$group] = array($name);
                }
             }
          }
   }

# in case this is not defined, set to LOCALTIME so uptime will be 0 in the display
if ( !isset($metrics['boottime']['VAL']) ){ $metrics['boottime']['VAL'] = $cluster['LOCALTIME'];}

# Add the uptime metric for this host. Cannot be done in ganglia.php,
# since it requires a fully-parsed XML tree. The classic contructor problem.
$s_metrics['uptime']['TYPE'] = "string";
$s_metrics['uptime']['VAL'] = uptime($cluster['LOCALTIME'] - $metrics['boottime']['VAL']);
$s_metrics['uptime']['TITLE'] = "Uptime";

# Add the gmond started timestamps & last reported time (in uptime format) from
# the HOST tag:
$s_metrics['gmond_started']['TYPE'] = "timestamp";
$s_metrics['gmond_started']['VAL'] = $hosts_up['GMOND_STARTED'];
$s_metrics['gmond_started']['TITLE'] = "Gmond Started";
$s_metrics['last_reported']['TYPE'] = "string";
$s_metrics['last_reported']['VAL'] = uptime($cluster['LOCALTIME'] - $hosts_up['REPORTED']);
$s_metrics['last_reported']['TITLE'] = "Last Reported";

$s_metrics['ip_address']['TITLE'] = "IP Address";
$s_metrics['ip_address']['VAL'] = $hosts_up['IP'];
$s_metrics['ip_address']['TYPE'] = "string";
$s_metrics['location']['TITLE'] = "Location";
$s_metrics['location']['VAL'] = $hosts_up['LOCATION'];
$s_metrics['location']['TYPE'] = "string";

# Show string metrics
if (is_array($s_metrics))
   {
      $s_metrics_data = array();
      ksort($s_metrics);
      foreach ($s_metrics as $name => $v )
      {
        # RFM - If units aren't defined for metric, make it be the empty string
        ! array_key_exists('UNITS', $v) and $v['UNITS'] = "";
        if (isset($v['TITLE']))
           {
              $s_metrics_data[$name]["name"] = $v['TITLE'];
           }
        else
           {
              $s_metrics_data[$name]["name"] = $name;
           }
        if( $v['TYPE']=="timestamp" or (isset($always_timestamp[$name]) and $always_timestamp[$name]))
           {
              $s_metrics_data[$name]["value"] = date("r", $v['VAL']);
           }
        else
           {
              $s_metrics_data[$name]["value"] = $v['VAL'] . " " . $v['UNITS'];
           }
     }
   }
$data->assign("s_metrics_data", $s_metrics_data);

# Add the average node utilization to the time & string metrics section:
$avg_cpu_num = find_avg($clustername, $hostname, "cpu_num");
if ($avg_cpu_num == 0) $avg_cpu_num = 1;
$cluster_util = sprintf("%.0f", ((double) find_avg($clustername, $hostname, "load_one") / $avg_cpu_num ) * 100);
$data->assign("name", "Avg Utilization (last $range)");
$data->assign("value", "$cluster_util%");

# Show constant metrics.
if (isset($c_metrics) and is_array($c_metrics))
   {
      $c_metrics_data = array();
      ksort($c_metrics);
      foreach ($c_metrics as $name => $v )
     {
        if (isset($v['TITLE']))
           {
              $c_metrics_data[$name]["name"] =  $v['TITLE'];
           }
        else
           {
              $c_metrics_data[$name]["name"] = $name;
           }
        $c_metrics_data[$name]["value"] = "$v[VAL] $v[UNITS]";
     }
   }
$data->assign("c_metrics_data", $c_metrics_data);

# Show graphs.
if ( is_array($g_metrics) && is_array($g_metrics_group) )
   {
      $g_metrics_group_data = array();
      ksort($g_metrics_group);

      foreach ( $g_metrics_group as $group => $metric_array )
         {
            if ( $group == "" ) {
               $group = "no_group";
            }
            $c = count($metric_array);
            $g_metrics_group_data[$group]["group_metric_count"] = $c;
            $i = 0;
            ksort($g_metrics);
            foreach ( $g_metrics as $name => $v )
               {
                  if ( in_array($name, $metric_array) ) {
                     $g_metrics_group_data[$group]["metrics"][$name]["graphargs"] = $v['graph'];
                     $g_metrics_group_data[$group]["metrics"][$name]["alt"] = "$hostname $name";
                     $g_metrics_group_data[$group]["metrics"][$name]["desc"] = "";
                     if (isset($v['description']))
                        $g_metrics_group_data[$group]["metrics"][$name]["desc"] = $v['description'];
                     $g_metrics_group_data[$group]["metrics"][$name]["new_row"] = "";
                     if ( !(++$i % $metriccols) && ($i != $c) )
                        $g_metrics_group_data[$group]["metrics"][$name]["new_row"] = "</TR><TR>";
                  }
               }
         }

   }
$data->assign("g_metrics_group_data", $g_metrics_group_data);
$dwoo->output($tpl, $data);
?>
