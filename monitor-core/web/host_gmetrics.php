<?php
/* $Id$ */

$clustername = escapeshellcmd(rawurldecode($_GET["c"]));
$hostname = escapeshellcmd(rawurldecode($_GET["h"]));
$context = "host";

include_once "conf.php";
include_once "ganglia.php";
include_once "functions.php";
include_once "get_ganglia.php";
include_once "./class.TemplatePower.inc.php";

# RFM - These lines prevent "undefined variable" error messages in 
# the ssl_error_log file.
if(!isset($always_timestamp)) $always_timestamp = array();

# RFM - Quoted the array indices to suppress error messages in 
# ssl_error_log file.

function byTN($a, $b)
{
   $aTN = $a['TN'];
   $bTN = $b['TN'];
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
      if ($v['SOURCE'] == "gmetric") {
         $g_metrics[$name] = $v;
      }
   }

# Show gmetrics
if (isset($g_metrics) and is_array($g_metrics))
   {
      uasort($g_metrics, "byTN");
      foreach ($g_metrics as $name => $v )
      {
       #  echo "Adding gmetric name $name<br>";
        $tpl->newBlock("g_metric_info");
        $tpl->assign("name", $name);
        $tpl->assign("tn", $v['TN']);
        $tpl->assign("tmax", $v['TMAX']);
        $tpl->assign("dmax", $v['DMAX']);
	# RFM - Added isset() call to avoid "undefined index" errors in
	# the ssl_error_log file.
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

$tpl->printToScreen();
?>
