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

$tpl = new Dwoo_Template_File( template("physical_view.tpl") );
$data = new Dwoo_Data();
$data->assign("cluster",$clustername);
$cluster_url=rawurlencode($clustername);
$data->assign("cluster_url",$cluster_url);

$verbosity_levels = array('3' => "", '2' => "", '1' => "");

# Assign the verbosity level. Can take the value of the 'p' CGI variable.
$verbose = $physical ? $physical : 2;

$verbosity_levels[$verbose] = "checked";
$data->assign("verbosity_levels", $verbosity_levels);

#
# Give the capacities of this cluster: total #CPUs, Memory, Disk, etc.
#
$CPUs = cluster_sum("cpu_num", $metrics);
# Divide by 1024^2 to get Memory in GB.
$Memory = sprintf("%.1f GB", cluster_sum("mem_total", $metrics)/(float)1048576);
$Disk = cluster_sum("disk_total", $metrics);
$Disk = $Disk ? sprintf("%.1f GB", $Disk) : "Unknown"; 
list($most_full, $most_full_host) = cluster_max("part_max_used", $metrics);
$data->assign("CPUs", $CPUs);
$data->assign("Memory", $Memory);
$data->assign("Disk", $Disk);

# Show which node has the most full disk.
$most_full_hosturl=rawurlencode($most_full_host);
$most_full = $most_full ? "<a href=\"./?p=1&amp;c=$cluster_url&amp;h=$most_full_host\">".
   "$most_full_host ($most_full% Used)</a>" : "Unknown";
$data->assign("most_full", $most_full);
$data->assign("cols_menu", $cols_menu);


#-------------------------------------------------------------------------------
# Displays a rack and all its nodes.
function showrack($ID)
{
   global $verbose, $racks, $racks_data, $metrics, $cluster, $hosts_up, $hosts_down;
   global $cluster_url, $tpl, $clusters;

   $racks_data[$ID]["RackID"] = "";

   if ($ID>=0) {
      $racks_data[$ID]["RackID"] = "<tr><th>Rack $ID</th></tr>";
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
$racks_data = array();

# Make a $cols-wide table of Racks.
$i=1;
foreach ($racks as $rack=>$v)
   {
      $racknodes = showrack($rack);

      $racks_data[$rack]["nodes"] = $racknodes;
      $racks_data[$rack]["tr"] = "";

      if (! ($i++ % $hostcols)) {
         $racks_data["tr"] = "</tr><tr>";
      }
   }

$data->assign("racks", $racks_data);
$dwoo->output($tpl, $data);
?>
