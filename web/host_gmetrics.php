<?php
/* $Id$ */

$clustername = escapeshellcmd(rawurldecode($HTTP_GET_VARS["c"]));
$hostname = escapeshellcmd(rawurldecode($HTTP_GET_VARS["h"]));
$context = "host";

include_once "conf.php";
include_once "ganglia.php";
include_once "functions.php";
include_once "get_ganglia.php";
include_once "./class.TemplatePower.inc.php";

function byTN($a, $b)
{
   $aTN = $a[TN];
   $bTN = $b[TN];
   if ($aTN == $bTN)
      return 0;
   return ($aTN < $bTN) ? -1 : 1;
}

$tpl = new TemplatePower( template("host_gmetrics.tpl") );
$tpl->prepare();

$tpl->assign("cluster", $clustername);
$tpl->assign("host", $hostname);
$tpl->assign("node_image", node_image($metrics));

if($hosts_up)
      $tpl->assign("node_msg", "This host is up and running."); 
else
      $tpl->assign("node_msg", "This host is down."); 

$cluster_url=rawurlencode($clustername);
$tpl->assign("cluster_url", $cluster_url);

# For the node view link.
$tpl->assign("host_view","./?c=$cluster_url&amp;h=$hostname");

foreach ($metrics as $name => $v)
   {
      # Show only user defined metrics.
      if ($v[SOURCE] == "gmetric") {
         $g_metrics[$name] = $v;
      }
   }

# Show gmetrics
if (is_array($g_metrics))
   {
      uasort($g_metrics, "byTN");
      foreach ($g_metrics as $name => $v )
      {
       #  echo "Adding gmetric name $name<br>";
        $tpl->newBlock("g_metric_info");
        $tpl->assign("name", $name);
        $tpl->assign("tn", $v[TN]);
        $tpl->assign("tmax", $v[TMAX]);
        $tpl->assign("dmax", $v[DMAX]);
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

$tpl->printToScreen();
?>
