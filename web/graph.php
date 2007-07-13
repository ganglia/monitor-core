<?php
/* $Id$ */
include_once "./conf.php";
include_once "./get_context.php";

# RFM - Added all the isset() tests to eliminate "undefined index"
# messages in ssl_error_log.

# Graph specific variables
$size = isset($_GET["z"]) ?
	escapeshellcmd( rawurldecode( $_GET["z"] )) : NULL;
$graph = isset($_GET["g"]) ?
	escapeshellcmd( rawurldecode( $_GET["g"] )) : NULL;
$grid = isset($_GET["G"]) ?
	escapeshellcmd( rawurldecode( $_GET["G"] )) : NULL;
$self = isset($_GET["me"]) ?
	escapeshellcmd(rawurldecode($_GET["me"])) : NULL;
$max = isset($_GET["x"]) ? 
	escapeshellcmd( rawurldecode($_GET["x"] )) : NULL;
$min = isset($_GET["n"]) ?
	escapeshellcmd( rawurldecode($_GET["n"] )) : NULL;
$value = isset($_GET["v"]) ?
	escapeshellcmd( rawurldecode( $_GET["v"] )) : NULL;
$load_color = isset($_GET["l"]) ?
	escapeshellcmd( rawurldecode( $_GET["l"] )) : NULL;
$vlabel = isset($_GET["vl"]) ?
	escapeshellcmd( rawurldecode( $_GET["vl"] )) : NULL;
$sourcetime = isset($_GET["st"]) ?
	escapeshellcmd($_GET["st"]) : NULL;

# RFM - Define these variables to avoid "Undefined variable" errors being 
# reported in ssl_error_log.
$command = "";
$extras = "";
$upper_limit = "";
$lower_limit = "";
$background = "";
$vertical_label = "";

# Assumes we have a $start variable (set in get_context.php).
if ($size == "small")
    {
      $height = 40;
      $width = 130;
      $fudge_0 = 0;
      $fudge_1 = 0;
      $fudge_2 = 0;
    }
else if ($size == "medium")
    {
      $height = 75;
      $width = 300;
      $fudge_0 = 0;
      $fudge_1 = 14;
      $fudge_2 = 28;
    }
else if ($size == "large")
    {
      $height = 600;
      $width = 800;
    }
else if ($size == "xlarge")
    {
      $height = 1200;
      $width = 1600;
    }
else
    {
      $height = 100;
      $width = 400;
      $fudge_0 = 0;
      $fudge_1 = 0;
      $fudge_2 = 0;
    }


# This security fix was brought to my attention by Peter Vreugdenhil <petervre@sci.kun.nl>
# Dont want users specifying their own malicious command via GET variables e.g.
# http://ganglia.mrcluster.org/graph.php?graph=blob&command=whoami;cat%20/etc/passwd
#
if($command)
    {
      exit();
    }

switch ($context)
{
    case "meta":
      $rrd_dir = "$rrds/__SummaryInfo__";
      break;
    case "grid":
      $rrd_dir = "$rrds/$grid/__SummaryInfo__";
      break;
    case "cluster":
      $rrd_dir = "$rrds/$clustername/__SummaryInfo__";
      break;
    case "host":
      $rrd_dir = "$rrds/$clustername/$hostname";
      break;
    default:
      exit;
}

$fudge = 0;
if ($graph)   /* Canned graph request */
    {
      if($graph == "cpu_report")
         {
            $fudge = $fudge_1;
            $style = "CPU";

            $upper_limit = "--upper-limit 100 --rigid";
            $lower_limit = "--lower-limit 0";

            $vertical_label = "--vertical-label Percent ";

            if($context != "host" )
               {
                  /* If we are not in a host context, then we need to calculate the average */
                  $series =
                  "DEF:'num_nodes'='${rrd_dir}/cpu_user.rrd':'num':AVERAGE "
                  ."DEF:'cpu_user'='${rrd_dir}/cpu_user.rrd':'sum':AVERAGE "
                  ."CDEF:'ccpu_user'=cpu_user,num_nodes,/ "
                  ."DEF:'cpu_nice'='${rrd_dir}/cpu_nice.rrd':'sum':AVERAGE "
                  ."CDEF:'ccpu_nice'=cpu_nice,num_nodes,/ "
                  ."DEF:'cpu_system'='${rrd_dir}/cpu_system.rrd':'sum':AVERAGE "
                  ."CDEF:'ccpu_system'=cpu_system,num_nodes,/ "
                  ."DEF:'cpu_idle'='${rrd_dir}/cpu_idle.rrd':'sum':AVERAGE "
                  ."CDEF:'ccpu_idle'=cpu_idle,num_nodes,/ "
                  ."AREA:'ccpu_user'#$cpu_user_color:'User CPU' "
                  ."STACK:'ccpu_nice'#$cpu_nice_color:'Nice CPU' "
                  ."STACK:'ccpu_system'#$cpu_system_color:'System CPU' ";
                  if (file_exists("$rrd_dir/cpu_wio.rrd")) {
                     $series .= "DEF:'cpu_wio'='${rrd_dir}/cpu_wio.rrd':'sum':AVERAGE "
                     ."CDEF:'ccpu_wio'=cpu_wio,num_nodes,/ "
                     ."STACK:'ccpu_wio'#$cpu_wio_color:'WAIT CPU' ";
                  }
                  $series .= "STACK:'ccpu_idle'#$cpu_idle_color:'Idle CPU' ";
               }
            else
               {
                  $series ="DEF:'cpu_user'='${rrd_dir}/cpu_user.rrd':'sum':AVERAGE "
                  ."DEF:'cpu_nice'='${rrd_dir}/cpu_nice.rrd':'sum':AVERAGE "
                  ."DEF:'cpu_system'='${rrd_dir}/cpu_system.rrd':'sum':AVERAGE "
                  ."DEF:'cpu_idle'='${rrd_dir}/cpu_idle.rrd':'sum':AVERAGE "
                  ."AREA:'cpu_user'#$cpu_user_color:'User CPU' "
                  ."STACK:'cpu_nice'#$cpu_nice_color:'Nice CPU' "
                  ."STACK:'cpu_system'#$cpu_system_color:'System CPU' ";
                  if (file_exists("$rrd_dir/cpu_wio.rrd")) {
                     $series .= "DEF:'cpu_wio'='${rrd_dir}/cpu_wio.rrd':'sum':AVERAGE "
                     ."STACK:'cpu_wio'#$cpu_wio_color:'WAIT CPU' ";
                  }
                  $series .= "STACK:'cpu_idle'#$cpu_idle_color:'Idle CPU' ";
               }
         }
      else if ($graph == "mem_report")
         {
            $fudge = $fudge_0;
            $style = "Memory";

            $lower_limit = "--lower-limit 0 --rigid";
            $extras = "--base 1024";
            $vertical_label = "--vertical-label Bytes";

            $series = "DEF:'mem_total'='${rrd_dir}/mem_total.rrd':'sum':AVERAGE "
               ."CDEF:'bmem_total'=mem_total,1024,* "
               ."DEF:'mem_shared'='${rrd_dir}/mem_shared.rrd':'sum':AVERAGE "
               ."CDEF:'bmem_shared'=mem_shared,1024,* "
               ."DEF:'mem_free'='${rrd_dir}/mem_free.rrd':'sum':AVERAGE "
               ."CDEF:'bmem_free'=mem_free,1024,* "
               ."DEF:'mem_cached'='${rrd_dir}/mem_cached.rrd':'sum':AVERAGE "
               ."CDEF:'bmem_cached'=mem_cached,1024,* "
               ."DEF:'mem_buffers'='${rrd_dir}/mem_buffers.rrd':'sum':AVERAGE "
               ."CDEF:'bmem_buffers'=mem_buffers,1024,* "
               ."CDEF:'bmem_used'='bmem_total','bmem_shared',-,'bmem_free',-,'bmem_cached',-,'bmem_buffers',- "
               ."AREA:'bmem_used'#$mem_used_color:'Memory Used' "
               ."STACK:'bmem_shared'#$mem_shared_color:'Memory Shared' "
               ."STACK:'bmem_cached'#$mem_cached_color:'Memory Cached' "
               ."STACK:'bmem_buffers'#$mem_buffered_color:'Memory Buffered' ";
            if (file_exists("$rrd_dir/swap_total.rrd")) {
               $series .= "DEF:'swap_total'='${rrd_dir}/swap_total.rrd':'sum':AVERAGE "
               ."DEF:'swap_free'='${rrd_dir}/swap_free.rrd':'sum':AVERAGE "
               ."CDEF:'bmem_swapped'='swap_total','swap_free',-,1024,* "
               ."STACK:'bmem_swapped'#$mem_swapped_color:'Memory Swapped' ";
            }
            $series .= "LINE2:'bmem_total'#$cpu_num_color:'Total In-Core Memory' ";
         }
      else if ($graph == "load_report")
         {
            $fudge = $fudge_2;
            $style = "Load";

            $lower_limit = "--lower-limit 0 --rigid";
            $vertical_label = "--vertical-label 'Load/Procs'";

            $series = "DEF:'load_one'='${rrd_dir}/load_one.rrd':'sum':AVERAGE "
               ."DEF:'proc_run'='${rrd_dir}/proc_run.rrd':'sum':AVERAGE "
               ."DEF:'cpu_num'='${rrd_dir}/cpu_num.rrd':'sum':AVERAGE ";
            if( $context != "host" )
               {
                  $series .="DEF:'num_nodes'='${rrd_dir}/cpu_num.rrd':'num':AVERAGE ";
               }
            $series .="AREA:'load_one'#$load_one_color:'1-min Load' ";
            if( $context != "host" )
               {
                  $series .= "LINE2:'num_nodes'#$num_nodes_color:'Nodes' ";
               }
            $series .="LINE2:'cpu_num'#$cpu_num_color:'CPUs' ";
            $series .="LINE2:'proc_run'#$proc_run_color:'Running Processes' ";
         }
      else if ($graph == "network_report")
         {
            $fudge = $fudge_2;
            $style = "Network";

            $lower_limit = "--lower-limit 0 --rigid";
            $extras = "--base 1024";
            $vertical_label = "--vertical-label 'Bytes/sec'";

            $series = "DEF:'bytes_in'='${rrd_dir}/bytes_in.rrd':'sum':AVERAGE "
               ."DEF:'bytes_out'='${rrd_dir}/bytes_out.rrd':'sum':AVERAGE "
               ."LINE2:'bytes_in'#$mem_cached_color:'In' "
               ."LINE2:'bytes_out'#$mem_used_color:'Out' ";
         }
      else if ($graph == "packet_report")
         {
            $fudge = $fudge_2;
            $style = "Packets";

            $lower_limit = "--lower-limit 0 --rigid";
            $extras = "--base 1024";
            $vertical_label = "--vertical-label 'Packets/sec'";

            $series = "DEF:'bytes_in'='${rrd_dir}/pkts_in.rrd':'sum':AVERAGE "
               ."DEF:'bytes_out'='${rrd_dir}/pkts_out.rrd':'sum':AVERAGE "
               ."LINE2:'bytes_in'#$mem_cached_color:'In' "
               ."LINE2:'bytes_out'#$mem_used_color:'Out' ";
         }
      else
         {
            /* Got a strange value for $graph */
            exit();
         }
    }
else
    {
      /* Custom graph */
      $style = "";

      $subtitle = $metricname;
      if ($context == "host") 
      {
          if ($size == "small")
              $prefix = $metricname;
          else
              $prefix = $hostname;

          $value = $value>1000 ? number_format($value) : number_format($value, 2);

          if ($range=="job") {
               $hrs = intval( -$jobrange / 3600 );
               $subtitle = "$prefix last ${hrs}h (now $value)";
          }
          else
              $subtitle = "$prefix last $range (now $value)";
      }

      if (is_numeric($max))
         $upper_limit = "--upper-limit '$max' ";
      if (is_numeric($min))
         $lower_limit ="--lower-limit '$min' ";

      if ($vlabel)
         $vertical_label = "--vertical-label '$vlabel'";
      else if ($upper_limit or $lower_limit)
         {
            $max = $max>1000 ? number_format($max) : number_format($max, 2);
            $min = $min>0 ? number_format($min,2) : $min;

            $vertical_label ="--vertical-label '$min - $max' ";
         }

      $rrd_file = "$rrd_dir/$metricname.rrd";
      $series = "DEF:'sum'='$rrd_file':'sum':AVERAGE "
         ."AREA:'sum'#$default_metric_color:'$subtitle' ";
      if ($jobstart)
         $series .= "VRULE:$jobstart#$jobstart_color ";
    }

# Set the graph title.
if($context == "meta")
   {
     $title = "$self $meta_designator $style last $range";
   }
else if ($context == "grid")
  {
     $title = "$grid $meta_designator $style last $range";
  }
else if ($context == "cluster")
   {
      $title = "$clustername $style last $range";
   }
else
   {
    if ($size == "small")
      {
        # Value for this graph define a background color.
        if (!$load_color) $load_color = "ffffff";
        $background = "--color BACK#'$load_color'";

        $title = $hostname;
      }
    else if ($style)
       $title = "$hostname $style last $range";
    else
       $title = $metricname;
   }

# Calculate time range.
if ($sourcetime)
   {
      $end = $sourcetime;
      # Get_context makes start negative.
      $start = $sourcetime + $start;
   }
# Fix from Phil Radden, but step is not always 15 anymore.
if ($range=="month")
   $end = floor($end / 672) * 672;

#
# Generate the rrdtool graph command.
#
$fudge += $height;
$command = RRDTOOL . " graph - --start $start --end $end ".
   "--width $width --height $fudge $upper_limit $lower_limit ".
   "--title '$title' $vertical_label $extras $background ".
   $series;

$debug=0;

# Did we generate a command?   Run it.
if($command)
 {
   /*Make sure the image is not cached*/
   header ("Expires: Mon, 26 Jul 1997 05:00:00 GMT");   // Date in the past
   header ("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT"); // always modified
   header ("Cache-Control: no-cache, must-revalidate");   // HTTP/1.1
   header ("Pragma: no-cache");                     // HTTP/1.0
   if ($debug) {
     header ("Content-type: text/html");
     print "$command\n\n\n\n\n";
    }
   else {
     header ("Content-type: image/gif");
     passthru($command);
    }
 }

?>

