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
$tpl->assign("hostname", $hostname);

$graph_args = "h=$hostname&amp;$get_metric_string&amp;st=$cluster[LOCALTIME]";

$optional_reports = "";


####################################################################################
# Let's find out what optional reports are included
# First we find out what the default (site-wide) reports are then look
# for host specific included or excluded reports
####################################################################################
$conf_dir = "./conf";

$default_reports = array("included_reports" => array(), "excluded_reports" => array());
if ( is_file($conf_dir . "/default.json") ) {
  $default_reports = array_merge($default_reports,json_decode(file_get_contents($conf_dir . "/default.json"), TRUE));
}

$host_file = $conf_dir . "/host_" . $hostname . ".json";
$override_reports = array("included_reports" => array(), "excluded_reports" => array());
if ( is_file($host_file) ) {
  $override_reports = array_merge($override_reports, json_decode(file_get_contents($host_file), TRUE));
}

# Merge arrays
$reports["included_reports"] = array_merge( $default_reports["included_reports"] , $override_reports["included_reports"]);
$reports["excluded_reports"] = array_merge($default_reports["excluded_reports"] , $override_reports["excluded_reports"]);

# Remove duplicates
$reports["included_reports"] = array_unique($reports["included_reports"]);
$reports["excluded_reports"] = array_unique($reports["excluded_reports"]);

foreach ( $reports["included_reports"] as $index => $report_name ) {

  if ( ! in_array( $report_name, $reports["excluded_reports"] ) ) {
    $optional_reports .= "<a name=metric_" . $report_name . ">
    <A HREF=\"./graph_all_periods.php?$graph_args&amp;g=" . $report_name . "&amp;z=large&amp;c=$cluster_url\">
    <IMG BORDER=0 ALT=\"$cluster_url\" SRC=\"./graph.php?$graph_args&amp;g=" . $report_name ."&amp;z=medium&amp;c=$cluster_url\"></A>
    <a style=\"background-color: #dddddd\" onclick=\"metricActions('" . $hostname . "','" . $report_name ."','graph'); return false;\" href=\"#\">+</a>
";
  }

}

$tpl->assign("optional_reports", $optional_reports);


if($hosts_up)
      $tpl->assign("node_msg", "This host is up and running."); 
else
      $tpl->assign("node_msg", "This host is down."); 

$cluster_url=rawurlencode($clustername);
$tpl->assign("cluster_url", $cluster_url);
$tpl->assign("graphargs", $graph_args);

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
$tpl->assign("size_menu", $size_menu);
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
if (isset($c_metrics) and is_array($c_metrics))
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
            $c = count($metric_array);
            $tpl->assign("group_metric_count", $c);
            $i = 0;
            ksort($g_metrics);
            foreach ( $g_metrics as $name => $v )
               {
                  if ( in_array($name, $metric_array) ) {
                     $tpl->newBlock("vol_metric_info");
                     $tpl->assign("graphargs", $v['graph']);
		     $tpl->assign("metric_name", $name);
		     $tpl->assign("host_name", $hostname);
                     $tpl->assign("alt", "$hostname $name");
                     if (isset($v['description']))
                        $tpl->assign("desc", $v['description']);
                     if ( !(++$i % $metriccols) && ($i != $c) )
                        $tpl->assign("new_row", "</TR><TR>");
                  }
               }
         }

   }

$tpl->printToScreen();
?>
