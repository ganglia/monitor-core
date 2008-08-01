<?php
# $Id$
#
# Shows information on a particular node, specified by
# node host name. Used in concert with the physical_view page.
# 
# Although this page repeats some information from
# host_view, it had the concept of Constant Metrics before
# the SLOPE=zero attribute.  It also uses style sheets for clean
# looks. In the future, it may display process information for
# the node as well.
#
# Originally by Federico Sacerdoti <fds@sdsc.edu>
#
# Host is specified in get_context.php.

if (empty($hostname)) {
   print "<h1>Missing a Node Name</h1>";
   return;
}

$tpl = new TemplatePower( template("show_node.tpl") );
$tpl->assignInclude("extra", template("node_extra.tpl"));
$tpl->prepare();

$up = $hosts_up ? 1 : 0;

$class = ($up) ? "even" : "down";
$tpl->assign("class",$class);
$tpl->assign("name", $hostname);

# $metrics is an array of [Metrics][Hostname][NAME|VAL|TYPE|UNITS|SOURCE].

# Find the host's physical location in the cluster.
$hostattrs = ($up) ? $hosts_up : $hosts_down;
list($rack,$rank,$plane) = findlocation($hostattrs);
$location = ($rack<0) ? "Unknown" : "Rack $rack, Rank $rank, Plane $plane.";
$tpl->assign("location",$location);

if(isset($hostattrs['ip'])) {
	$tpl->assign("ip", $hostattrs['ip']);
} else {
	$tpl->assign("ip", "");
}

# The metrics we need for this node.
$mem_total_gb = $metrics['mem_total']['VAL']/1048576;
$load_one=$metrics['load_one']['VAL'];
$load_five=$metrics['load_five']['VAL'];
$load_fifteen=$metrics['load_fifteen']['VAL'];
$cpu_user=$metrics['cpu_user']['VAL'];
$cpu_system=$metrics['cpu_system']['VAL'];
$cpu_idle=$metrics['cpu_idle']['VAL'];
$cpu_num=$metrics['cpu_num']['VAL'];
# Cannot be zero, since we use it as a divisor.
if (!$cpu_num) { $cpu_num=1; }
$cpu_speed=$metrics['cpu_speed']['VAL']/1024;
$disk_total=$metrics['disk_total']['VAL'];
$disk_free=$metrics['disk_free']['VAL'];
$disk_use = $disk_total - $disk_free;
$disk_units=$metrics['disk_total']['UNITS'];
$part_max_used=$metrics['part_max_used']['VAL'];
# Disk metrics are newer (as of 2.5.0), so we check more carefully.
$disk = ($disk_total) ? "Using $disk_use of $disk_total $disk_units" : "Unknown";
$part_max = ($part_max_used) ? "$part_max_used% used." : "Unknown";
   

# Compute time of last heartbeat from node's dendrite.
$clustertime=$cluster['LOCALTIME'];
$tpl->assign("clustertime", strftime("%c", $clustertime));
$heartbeat=$hostattrs['REPORTED'];
$age = $clustertime - $heartbeat;
if ($age > 3600) {
   $tpl->assign("age", uptime($age));
} else {
   $s = ($age > 1) ? "s" : "";
   $tpl->assign("age", "$age second$s");
}

# The these hardware units should be more flexible.
$s = ($cpu_num>1) ? "s" : "";
$tpl->assign("s",$s);
$tpl->assign("cpu", sprintf("%s x %.2f GHz", $cpu_num, $cpu_speed));
$tpl->assign("mem", sprintf("%.2f GB", $mem_total_gb));
$tpl->assign("disk","$disk");
$tpl->assign("part_max_used", "$part_max");
$tpl->assign("load_one",$load_one);
$tpl->assign("load_five",$load_five);
$tpl->assign("load_fifteen",$load_fifteen);
$tpl->assign("cpu_user",$cpu_user);
$tpl->assign("cpu_system",$cpu_system);
$tpl->assign("cpu_idle",$cpu_idle);

# Choose a load color from a unix load value.
function loadindex($load) {
   global $cpu_num;
   # Highest color comes at a load of loadscalar*10.
   $loadscalar=0.2;
   $level=intval($load/($loadscalar*$cpu_num))+1;
   # Trim level to a max of 10.
   $level = $level > 10 ? "L10" : "L$level";
   return $level;
}

# Choose a load color from a 0-100 percentage.
function percentindex($val) {
   $level = intval($val/10 + 1);
   $level = $level>10 ? "L10" : "L$level";
   return $level;
}

$tpl->assign("load1",loadindex($load_one));
$tpl->assign("load5",loadindex($load_five));
$tpl->assign("load15",loadindex($load_fifteen));
$tpl->assign("user",percentindex($cpu_user));
$tpl->assign("sys",percentindex($cpu_system));
$tpl->assign("idle",percentindex(100 - $cpu_idle));

# Software metrics
$os_name=$metrics['os_name']['VAL'];
$os_release=$metrics['os_release']['VAL'];
$machine_type=$metrics['machine_type']['VAL'];
$boottime=$metrics['boottime']['VAL'];
$booted=date("F j, Y, g:i a", $boottime);
$uptime=uptime($cluster['LOCALTIME'] - $metrics['boottime']['VAL']);

# Turning into MBs. A MB is 1024 bytes.
$swap_free=$metrics['swap_free']['VAL']/1024.0;
$swap_total=sprintf("%.1f", $metrics['swap_total']['VAL']/1024.0);
$swap_used=sprintf("%.1f", $swap_total - $swap_free);

$tpl->assign("OS","$os_name $os_release ($machine_type)");
$tpl->assign("booted","$booted");
$tpl->assign("uptime", $up ? $uptime : "[down]");
$tpl->assign("swap","Using $swap_used of $swap_total MB swap.");

# For the back link.
$cluster_url=rawurlencode($clustername);
$tpl->assign("physical_view","./?p=$physical&amp;c=$cluster_url");

# For the full host view link.
$tpl->assign("full_host_view","./?c=$cluster_url&amp;h=$hostname&amp;$get_metric_string");

# For the reload link.
$tpl->assign("self","./?c=$cluster_url&amp;h=$hostname&amp;p=$physical");

$tpl->printToScreen();
?>
