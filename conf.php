<?php
# $Id$
#
# Gmetad-webfrontend version. Used to check for updates.
#
$majorversion = 2;
$minorversion = 5;
$microversion = 5;
#
# The name of the directory in "./templates" which contains the
# templates that you want to use. Templates are like a skin for the
# site that can alter its look and feel.
#
$template_name = "default";

#
# If you installed gmetad in a directory other than the default
# make sure you change it here.
#

# The original Perl gmetad.
#$gmetad_root = "/usr/local/gmetad";

# The high-performance gmetad.
$gmetad_root = "/var/lib/ganglia";
$rrds = "$gmetad_root/rrds";

# Leave this alone if rrdtool is installed in $gmetad_root,
# otherwise, change it if it is installed elsewhere (like /usr/bin)
define("RRDTOOL", "/usr/bin/rrdtool");

#
# If you want to grab data from a different ganglia source specify it here.
# Although, it would be strange to alter the IP since the Round-Robin
# databases need to be local to be read. 
#
$ganglia_ip = "127.0.0.1";
$ganglia_port = 8652;

# Old-style names.
$gmetad_ip = $ganglia_ip;
$gmetad_port = $ganglia_port;

#
# The maximum number of dynamic graphs to display.  If you set this
# to 0 (the default) all graphs will be shown.  This option is
# helpful if you are viewing the web pages from a browser with a 
# small pipe.
#
$max_graphs = 0;

#
# Turn on and off the Grid Snapshot. Now that we have a
# hierarchical snapshot (per-cluster instead of per-node) on
# the meta page this makes more sense. Most people will want this
# on.
#
$show_meta_snapshot = "yes";

# 
# The default refresh frequency on pages.
#
$default_refresh = 300;

#
# Colors for the CPU report graph
#
$cpu_user_color = "3333bb";
$cpu_nice_color = "ffea00";
$cpu_system_color = "dd0000";
$cpu_idle_color = "f2f2f2";

#
# Colors for the MEMORY report graph
#
$mem_used_color = "5555cc";
$mem_shared_color = "0000aa";
$mem_cached_color = "33cc33";
$mem_buffered_color = "99ff33";
$mem_free_color = "00ff00";
$mem_swapped_color = "9900CC";

#
# Colors for the LOAD report graph
#
$load_one_color = "CCCCCC";
$proc_run_color = "0000FF";
$cpu_num_color  = "FF0000";
$num_nodes_color = "00FF00";

# Other colors
$jobstart_color = "ff3300";

#
# Colors for the load ranks.
#
$load_colors = array(
   "100+" => "ff634f",
   "75-100" =>"ffa15e",
   "50-75" => "ffde5e",
   "25-50" => "caff98",
   #"0-25" => "efefef",
   "0-25" => "e2ecff",
   "down" => "515151"
);


#
# Default color for single metric graphs
#
$default_metric_color = "555555";

#
# Default graph range (hour, day, week, month, or year)
#
$default_range = "hour";

#
# Default metric 
#
$default_metric = "load_one";
?>
