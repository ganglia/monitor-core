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

$tpl->assign("ip", $hosts_up[IP]);

foreach ($metrics as $name => $v)
   {
       if ($v[TYPE] == "string" or $v[TYPE]=="timestamp" or $always_timestamp[$name])
          {
             # Long gmetric name/values will disrupt the display here.
             if ($v[SOURCE] == "gmond") $s_metrics[$name] = $v;
          }
       elseif ($v[SLOPE] == "zero" or $always_constant[$name])
          {
             $c_metrics[$name] = $v;
          }
       else if ($reports[$metric])
          continue;
       else
          {
             $graphargs = "c=$cluster_url&amp;h=$hostname&amp;v=$v[VAL]"
               ."&amp;m=$name&amp;r=$range&amp;z=medium&amp;jr=$jobrange"
               ."&amp;js=$jobstart&amp;st=$cluster[LOCALTIME]";
             # Adding units to graph 2003 by Jason Smith <smithj4@bnl.gov>.
             if ($v[UNITS]) {
                $encodeUnits = rawurlencode($v[UNITS]);
                $graphargs .= "&vl=$encodeUnits";
             }
             $g_metrics[$name][graph] = $graphargs;
          }
   }
# Add the uptime metric for this host. Cannot be done in ganglia.php,
# since it requires a fully-parsed XML tree. The classic contructor problem.
$s_metrics[uptime][TYPE] = "string";
$s_metrics[uptime][VAL] = uptime($cluster[LOCALTIME] - $metrics[boottime][VAL]);

# Add the gmond started timestamps & last reported time (in uptime format) from
# the HOST tag:
$s_metrics[gmond_started][TYPE] = "timestamp";
$s_metrics[gmond_started][VAL] = $hosts_up[GMOND_STARTED];
$s_metrics[last_reported][TYPE] = "string";
$s_metrics[last_reported][VAL] = uptime($cluster[LOCALTIME] - $hosts_up[REPORTED]);

# Show string metrics
if (is_array($s_metrics))
   {
      ksort($s_metrics);
      foreach ($s_metrics as $name => $v )
     {
        $tpl->newBlock("string_metric_info");
        $tpl->assign("name", $name);
        if( $v[TYPE]=="timestamp" or $always_timestamp[$name])
           {
              $tpl->assign("value", date("r", $v[VAL]));
           }
        else
           {
              $tpl->assign("value", "$v[VAL] $v[UNITS]");
           }
     }
   }

# Show constant metrics.
if (is_array($c_metrics))
   {
      ksort($c_metrics);
      foreach ($c_metrics as $name => $v )
     {
        $tpl->newBlock("const_metric_info");
        $tpl->assign("name", $name);
        $tpl->assign("value", "$v[VAL] $v[UNITS]");
     }
   }

# Show graphs.
if (is_array($g_metrics))
   {
      ksort($g_metrics);

      $i = 0;
      foreach ( $g_metrics as $name => $v )
         {
            $tpl->newBlock("vol_metric_info");
            $tpl->assign("graphargs", $v[graph]);
            $tpl->assign("alt", "$hostname $name");
            if($i++ %2)
               $tpl->assign("br", "<BR>");
         }
   }

$tpl->printToScreen();
?>
