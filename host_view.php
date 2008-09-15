<?php
/* $Id$ */

$tpl = new TemplatePower( template("host_view.tpl") );
$tpl->assignInclude("extra", template("host_extra.tpl"));
$tpl->prepare();

$tpl->assign("cluster", $clustername);
$tpl->assign("host", $hostname);
$tpl->assign("node_image", node_image($metrics));
$tpl->assign("sort",$sort);
$tpl->assign("range",$range);

if($hosts_up)
      $tpl->assign("node_msg", "This host is up and running."); 
else
      $tpl->assign("node_msg", "This host is down."); 

$cluster_url=rawurlencode($clustername);
$tpl->assign("cluster_url", $cluster_url);
$tpl->assign("graphargs", "h=$hostname&amp;$get_metric_string&amp;st=$cluster[LOCALTIME]");

# For the node view link.
$tpl->assign("node_view","./?p=2&amp;c=$cluster_url&amp;h=$hostname");

# No reason to go on if this node is down.
if ($hosts_down)
   {
      $tpl->printToScreen();
      return;
   }

$tpl->assign("ip", $hosts_up['IP']);
$tpl->newBlock('columns_dropdown');
$tpl->assign("metric_cols_menu", $metric_cols_menu);
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
             $graphargs = "c=$cluster_url&amp;h=$hostname&amp;v=$v[VAL]"
               ."&amp;m=$name&amp;r=$range&amp;z=medium&amp;jr=$jobrange"
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

# Show string metrics
if (is_array($s_metrics))
   {
      ksort($s_metrics);
      foreach ($s_metrics as $name => $v )
      {
        # RFM - If units aren't defined for metric, make it be the empty string
        ! array_key_exists('UNITS', $v) and $v['UNITS'] = "";
        $tpl->newBlock("string_metric_info");
        if (isset($v['TITLE']))
           {
              $tpl->assign("name", $v['TITLE']);
           }
        else
           {
              $tpl->assign("name", $name);
           }
        if( $v['TYPE']=="timestamp" or (isset($always_timestamp[$name]) and $always_timestamp[$name]))
           {
              $tpl->assign("value", date("r", $v['VAL']));
           }
        else
           {
              $tpl->assign("value", $v['VAL'] . " " . $v['UNITS']);
           }
     }
   }

# Add the average node utilization to the time & string metrics section:
$avg_cpu_num = find_avg($clustername, $hostname, "cpu_num");
if ($avg_cpu_num == 0) $avg_cpu_num = 1;
$cluster_util = sprintf("%.0f", ((double) find_avg($clustername, $hostname, "load_one") / $avg_cpu_num ) * 100);
$tpl->assign("name", "Avg Utilization (last $range)");
$tpl->assign("value", "$cluster_util%");

# Show constant metrics.
if (is_array($c_metrics))
   {
      ksort($c_metrics);
      foreach ($c_metrics as $name => $v )
     {
        $tpl->newBlock("const_metric_info");
        if (isset($v['TITLE']))
           {
              $tpl->assign("name", $v['TITLE']);
           }
        else
           {
              $tpl->assign("name", $name);
           }
        $tpl->assign("value", "$v[VAL] $v[UNITS]");
     }
   }

# Show graphs.
if ( is_array($g_metrics) && is_array($g_metrics_group) )
   {
      ksort($g_metrics_group);

      foreach ( $g_metrics_group as $group => $metric_array )
         {
            if ( $group == "" ) {
               $group = "no_group";
            }
            $tpl->newBlock("vol_group_info");
            $tpl->assign("group", $group);
            $tpl->assign("group_metric_count", count($metric_array));
            $i = 0;
            ksort($g_metrics);
            foreach ( $g_metrics as $name => $v )
               {
                  if ( in_array($name, $metric_array) ) {
                     $tpl->newBlock("vol_metric_info");
                     $tpl->assign("graphargs", $v['graph']);
                     $tpl->assign("alt", "$hostname $name");
                     if (isset($v['description']))
                        $tpl->assign("desc", $v['description']);
                     if ( !(++$i % $metriccols) )
                        $tpl->assign("br", "<BR>");
                  }
               }
         }

   }

$tpl->printToScreen();
?>
