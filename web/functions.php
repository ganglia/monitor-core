<?php

#
# Some common functions for the Ganglia PHP website.
# Assumes the Gmeta XML tree has already been parsed,
# and the global variables $metrics, $clusters, and $hosts
# have been set.
#

#-------------------------------------------------------------------------------
# Allows a form of inheritance for template files.
# If a file does not exist in the chosen template, the
# default is used. Cuts down on code duplication.
function template ($name)
{
   global $template_name;

   $fn = "./templates/$template_name/$name";
   $default = "./templates/default/$name";

   if (file_exists($fn)) {
      return $fn;
   }
   else {
      return $default;
   }
}

#-------------------------------------------------------------------------------
# Creates a hidden input field in a form. Used to save CGI variables.
function hiddenvar ($name, $var)
{

   $hidden = "";
   if ($var) {
      #$url = rawurlencode($var);
      $hidden = "<input type=\"hidden\" name=\"$name\" value=\"$var\">\n";
   }
   return $hidden;
}

#-------------------------------------------------------------------------------
# Gives a readable time string, from a "number of seconds" integer.
# Often used to compute uptime.
function uptime($uptimeS)
{
   $uptimeD=intval($uptimeS/86400);
   $uptimeS=$uptimeD ? $uptimeS % ($uptimeD*86400) : $uptimeS;
   $uptimeH=intval($uptimeS/3600);
   $uptimeS=$uptimeH ? $uptimeS % ($uptimeH*3600) : $uptimeS;
   $uptimeM=intval($uptimeS/60);
   $uptimeS=$uptimeM ? $uptimeS % ($uptimeM*60) : $uptimeS;

   $s = ($uptimeD!=1) ? "s" : "";
   return sprintf("$uptimeD day$s, %d:%02d:%02d",$uptimeH,$uptimeM,$uptimeS);
}

#-------------------------------------------------------------------------------
# Try to determine a nodes location in the cluster. Attempts to find the
# LOCATION attribute first. Requires the host attribute array from 
# $hosts[$cluster][$name], where $name is the hostname.
# Returns [-1,-1,-1] if we could not determine location.
#
function findlocation($attrs)
{
   $rack=$rank=$plane=-1;

   $loc=$attrs['LOCATION'];
   if ($loc) {
      sscanf($loc, "%d,%d,%d", $rack, $rank, $plane);
      #echo "Found LOCATION: $rack, $rank, $plane.<br>";
   }
   if ($rack<0 or $rank<0) {
      # Try to parse the host name. Assumes a compute-<rack>-<rank>
      # naming scheme.
      $n=sscanf($attrs['NAME'], "compute-%d-%d", $rack, $rank);
      $plane=0;
   }
   return array($rack,$rank,$plane);
}


#-------------------------------------------------------------------------------
function cluster_sum($name, $metrics)
{
   $sum = 0;

   foreach ($metrics as $host => $val)
      {
         if(isset($val[$name]['VAL'])) $sum += $val[$name]['VAL'];
      }

   return $sum;
}

#-------------------------------------------------------------------------------
function cluster_min($name, $metrics)
{
   $min = "";

   foreach ($metrics as $host => $val)
      {
         $v = $val[$name]['VAL'];
         if (!is_numeric($min) or $min < $v)
            {
               $min = $v;
               $minhost = $host;
            }
      }
   return array($min, $minhost);
}

#-------------------------------------------------------------------------------
#
# A useful function for giving the correct picture for a given
# load. Scope is "node | cluster | grid". Value is 0 <= v <= 1.
function load_image ($scope, $value)
{
   global $load_scale;

   $scaled_load = $value / $load_scale;
   if ($scaled_load>1.00) {
      $image = template("images/${scope}_overloaded.jpg");
   }
   else if ($scaled_load>=0.75) {
      $image = template("images/${scope}_75-100.jpg");
   }
   else if ($scaled_load >= 0.50) {
      $image = template("images/${scope}_50-74.jpg");
   }
   else if ($scaled_load>=0.25) {
      $image = template("images/${scope}_25-49.jpg");
   }
   else {
      $image = template("images/${scope}_0-24.jpg");
   }

   return $image;
}

#-------------------------------------------------------------------------------
# A similar function that specifies the background color for a graph
# based on load. Quantizes the load figure into 6 sets.
function load_color ($value)
{
   global $load_colors;
   global $load_scale;

   $scaled_load = $value / $load_scale;
   if ($scaled_load>1.00) {
      $color = $load_colors["100+"];
   }
   else if ($scaled_load>=0.75) {
      $color = $load_colors["75-100"];
   }
   else if ($scaled_load >= 0.50) {
      $color = $load_colors["50-75"];
   }
   else if ($scaled_load>=0.25) {
      $color = $load_colors["25-50"];
   }
   else if ($scaled_load < 0.0)
      $color = $load_colors["down"];
   else {
      $color = $load_colors["0-25"];
   }

   return $color;
}

#-------------------------------------------------------------------------------
#
# Just a useful function to print the HTML for
# the load/death of a cluster node
function node_image ($metrics)
{
   $cpu_num  = $metrics["cpu_num"]['VAL'];
   if(!$cpu_num || $cpu_num == 0)
      {
         $cpu_num = 1;
      }
   $load_one  = $metrics["load_one"]['VAL'];
   $value = $load_one / $cpu_num;

   # Check if the host is down
   # RFM - Added isset() check to eliminate error messages in ssl_error_log
   if (isset($hosts_down) and $hosts_down)
         $image = template("images/node_dead.jpg");
   else
         $image = load_image("node", $value);

   return $image;
}

#-------------------------------------------------------------------------------
#
# Finds the min/max over a set of metric graphs. Nodes is
# an array keyed by host names.
#
function find_limits($nodes, $metricname)
{
   global $metrics, $clustername, $rrds, $rrd_dir, $start, $end;

   if (!count($metrics))
      return array(0, 0);

   $firsthost = key($metrics);
   
   if (array_key_exists($metricname,$metrics[$firsthost])) {
     if ($metrics[$firsthost][$metricname]['TYPE'] == "string"
        or $metrics[$firsthost][$metricname]['SLOPE'] == "zero")
           return array(0,0);
   }
   else {
     return array(0,0);
   }

   $max=0;
   $min=0;
   foreach ( $nodes as $host => $value )
      {
         $out = array();

         $rrd_dir = "$rrds/$clustername/$host";
         if (file_exists("$rrd_dir/$metricname.rrd")) {
		$command = RRDTOOL . " graph - --start $start --end $end ".
		"DEF:limits='$rrd_dir/$metricname.rrd':'sum':AVERAGE ".
		"PRINT:limits:MAX:%.2lf ".
		"PRINT:limits:MIN:%.2lf";
		exec($command, $out);
		if(isset($out[1])) {
         		$thismax = $out[1];
	 	} else {
			$thismax = NULL;
	 	}
	 if (!is_numeric($thismax)) continue;
	 if ($max < $thismax) $max = $thismax;

	 $thismin=$out[2];
	 if (!is_numeric($thismin)) continue;
	 if ($min > $thismin) $min = $thismin;
	 #echo "$host: $thismin - $thismax (now $value)<br>\n";
	 }
      }
      
      return array($min, $max);
}

#-------------------------------------------------------------------------------
#
# Generates the colored Node cell HTML. Used in Physical
# view and others. Intended to be used to build a table, output
# begins with "<tr><td>" and ends the same.
function nodebox($hostname, $verbose, $title="", $extrarow="")
{
   global $cluster, $clustername, $metrics, $hosts_up, $GHOME;

   if (!$title) $title = $hostname;

   # Scalar helps choose a load color. The lower it is, the easier to get red.
   # The highest level occurs at a load of (loadscalar*10).
   $loadscalar=0.2;

   # An array of [NAME|VAL|TYPE|UNITS|SOURCE].
   $m=$metrics[$hostname];
   $up = $hosts_up[$hostname] ? 1 : 0;

   # The metrics we need for this node.

   # Give memory in Gigabytes. 1GB = 2^20 bytes.
   $mem_total_gb = $m['mem_total']['VAL']/1048576;
   $load_one=$m['load_one']['VAL'];
   $cpu_speed=$m['cpu_speed']['VAL']/1024;
   $cpu_num= $m['cpu_num']['VAL'];
   #
   # The nested tables are to get the formatting. Insane.
   # We have three levels of verbosity. At L3 we show
   # everything; at L1 we only show name and load.
   #
   $rowclass = $up ? rowStyle() : "down";
   $host_url=rawurlencode($hostname);
   $cluster_url=rawurlencode($clustername);
   
   $row1 = "<tr><td class=$rowclass>\n".
      "<table width=\"100%\" cellpadding=1 cellspacing=0 border=0><tr>".
      "<td><a href=\"$GHOME/?p=$verbose&amp;c=$cluster_url&amp;h=$host_url\">".
      "$title</a>&nbsp;<br>\n";

   $cpus = $cpu_num > 1 ? "($cpu_num)" : "";
   if ($up)
      $hardware = 
         sprintf("<em>cpu: </em>%.2f<small>G</small> %s ", $cpu_speed, $cpus) .
         sprintf("<em>mem: </em>%.2f<small>G</small>",$mem_total_gb);
   else $hardware = "&nbsp;";

   $row2 = "<tr><td colspan=2>";
   if ($verbose==2)
      $row2 .= $hardware;
   else if ($verbose > 2) {
      $hostattrs = $up ? $hosts_up : $hosts_down;
      $last_heartbeat = $hostattrs[$hostname]['TN'];
      $age = $last_heartbeat > 3600 ? uptime($last_heartbeat) : 
         "${last_heartbeat}s";
      $row2 .= "<font size=-2>Last heartbeat $age</font>";
      $row3 = $hardware;
   }

   #
   # Load box.
   #
   if (!$cpu_num) $cpu_num=1;
   $loadindex = intval($load_one / ($loadscalar*$cpu_num)) + 1;
   # 10 is currently the highest allowed load index.
   $load_class = $loadindex > 10 ? "L10" : "L$loadindex";
   $row1 .= "</td><td align=right valign=top>".
      "<table cellspacing=1 cellpadding=3 border=0><tr>".
      "<td class=$load_class align=right><small>$load_one</small>".
      "</td></tr></table>".
      "</td></tr>\n";

   # Construct cell.
   $cell = $row1;

   if ($extrarow)
      $cell .= $extrarow;

   if ($verbose>1)
      $cell .= $row2;

   $cell .= "</td></tr></table>\n";
   # Tricky.
   if ($verbose>2)
      $cell .= $row3;

   $cell .= "</td></tr>\n";

   return $cell;
}

#-------------------------------------------------------------------------------
# Alternate between even and odd row styles.
function rowstyle()
{
   static $style;

   if ($style == "even") { $style = "odd"; }
   else { $style = "even"; }

   return $style;
}

#-------------------------------------------------------------------------------
# Organize hosts by rack locations.
# Works with or without "location" host attributes.
function physical_racks()
{
   global $hosts_up, $hosts_down;

   # 2Key = "Rack ID / Rank (order in rack)" = [hostname, UP|DOWN]
   $rack = NULL;

   # If we don't know a node's location, it goes in a negative ID rack.
   $i=1;
   $unknownID= -1;
   if (is_array($hosts_up)) {
      foreach ($hosts_up as $host=>$v) {
         # Try to find the node's location in the cluster.
         list($rack, $rank) = findlocation($v);

         if ($rack>=0 and $rank>=0) {
            $racks[$rack][$rank]=$v['NAME'];
            continue;
         }
         else {
            $i++;
            if (! ($i % 25)) {
               $unknownID--;
            }
            $racks[$unknownID][] = $v['NAME'];
         }
      }
   }
   if (is_array($hosts_down)) {
      foreach ($hosts_down as $host=>$v) {
         list($rack, $rank) = findlocation($v);
         if ($rack>=0 and $rank>=0) {
            $racks[$rack][$rank]=$v['NAME'];
            continue;
         }
         else {
            $i++;
            if (! ($i % 25)) {
               $unknownID--;
            }
            $racks[$unknownID][] = $v['NAME'];
         }
      }
   }

   # Sort the racks array.
   if ($unknownID<-1) { krsort($racks); }
   else {
      ksort($racks);
      reset($racks);
      while (list($rack,) = each($racks)) {
         # In our convention, y=0 is close to the floor. (Easier to wire up)
         krsort($racks[$rack]);
      }
   }
   
   return $racks;
}

?>
