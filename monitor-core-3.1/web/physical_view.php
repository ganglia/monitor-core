<?php
/* $Id$ */
#
# Displays the cluster in a physical view. Cluster nodes in
# this view are located by Rack, Rank, and Plane in the physical
# cluster.
#
# Originally written by Federico Sacerdoti <fds@sdsc.edu>
# Part of the Ganglia Project, All Rights Reserved.
#

# Called from index.php, so cluster and xml tree vars
# ($metrics, $clusters, $hosts) are set, and header.php
# already called.

$tpl = new TemplatePower( template("physical_view.tpl") );
$tpl->prepare();
$tpl->assign("cluster",$clustername);
$cluster_url=rawurlencode($clustername);
$tpl->assign("cluster_url",$cluster_url);

# Assign the verbosity level. Can take the value of the 'p' CGI variable.
$verbose = $physical ? $physical : 2;

# The name of the variable changes based on the level. Nice.
$tpl->assign("checked$verbose","checked");

#
# Give the capacities of this cluster: total #CPUs, Memory, Disk, etc.
#
$CPUs = cluster_sum("cpu_num", $metrics);
# Divide by 1024^2 to get Memory in GB.
$Memory = sprintf("%.1f GB", cluster_sum("mem_total", $metrics)/(float)1048576);
$Disk = cluster_sum("disk_total", $metrics);
$Disk = $Disk ? sprintf("%.1f GB", $Disk) : "Unknown"; 
list($most_full, $most_full_host) = cluster_min("part_max_used", $metrics);
$tpl->assign("CPUs", $CPUs);
$tpl->assign("Memory", $Memory);
$tpl->assign("Disk", $Disk);

# Show which node has the most full disk.
$most_full_hosturl=rawurlencode($most_full_host);
$most_full = $most_full ? "<a href=\"./?p=1&amp;c=$cluster_url&amp;h=$most_full_host\">".
   "$most_full_host ($most_full% Used)</a>" : "Unknown";
$tpl->assign("most_full", $most_full);
$tpl->assign("cols_menu", $cols_menu);


#-------------------------------------------------------------------------------
# Displays a rack and all its nodes.
function showrack($ID)
{
   global $verbose, $racks, $metrics, $cluster, $hosts_up, $hosts_down;
   global $cluster_url, $tpl, $clusters;

   if ($ID>=0) {
      $tpl->assign("RackID","<tr><th>Rack $ID</th></tr>");
   }

   # A string of node HTML for the template.
   $nodes="";

   foreach ($racks[$ID] as $name)
   {
      $nodes .= nodebox($name, $verbose);
   }

   return $nodes;
}

#-------------------------------------------------------------------------------
#
# My Main
#

# 2Key = "Rack ID / Rank (order in rack)" = [hostname, UP|DOWN]
$racks = physical_racks();

# Make a $cols-wide table of Racks.
$i=1;
foreach ($racks as $rack=>$v)
   {
      $tpl->newBlock("racks");

      $racknodes = showrack($rack);

      $tpl->assign("nodes", $racknodes);

      if (! ($i++ % $hostcols)) {
         $tpl->assign("tr","</tr><tr>");
      }
   }

$tpl->printToScreen();

?>
