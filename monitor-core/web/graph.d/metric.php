<?php

// This report is used for specific metric graphs at the bottom of the cluster_view page.

/* Pass in by reference! */
function graph_metric ( &$rrdtool_graph ) {

    global $context, 
           $default_metric_color,
           $hostname,
           $jobstart,
           $load_color,
           $max,
           $meta_designator,
           $metricname,
           $min,
           $range,
           $rrd_dir,
           $size,
           $summary,
           $value,
           $vlabel;

    $rrdtool_graph['height']  +=  0 ; //no fudge needed

    switch ($context) {
    
    case 'host':
    
        if ($summary) {
            $rrdtool_graph['title'] = $hostname;
            $prefix = $metricname;
        }
        else {
            $rrdtool_graph['title'] = $metricname;
            $prefix = $hostname;
        }

        $prefix = $summary ? $metricname : $hostname;
        $value = $value > 1000 
                    ? number_format($value) 
                    : number_format($value,2);

        if ($range == 'job') {
            $hrs = intval (-$jobrange / 3600);
            $subtitle = "$prefix last ${hrs} (now $value)";
        } else {
            $subtitle = "$prefix last $range (now $value)";
        }

        break;
        
    case 'meta':
        $rrdtool_graph['title'] = "$meta_designator ". $rrdtool_graph['title'] ."last $range";;
        break;
        
    case 'grid':
        $rrdtool_graph['title'] = "$meta_designator ". $rrdtool_graph['title'] ."last $range";
        break;

    case 'cluster':
        $rrdtool_graph['title'] = "$clustername "    . $rrdtool_graph['title'] ."last $range";
        break;

    default:
        
        if ($size == 'small') {
            $rrdtool_graph['title'] = $hostname;
        } else if ($summary) {
            $rrdtool_graph['title'] = $hostname;
        } else {
            $rrdtool_graph['title'] = $metricname;
        }
        
        break;
        
    }

    if ($load_color) 
        $rrdtool_graph['color'] = "BACK#'$load_color'";

    if (isset($max) && is_numeric($max))
        $rrdtool_graph['upper-limit'] = $max;
    
    if (isset($min) && is_numeric($min))
        $rrdtool_graph['lower-limit'] = $min;
    
    if ($vlabel ) {
	// We should set $vlabel, even if it isn't used for spacing
	// and alignment reasons.  This is mostly for aesthetics
	$temp_vlabel = trim($vlabel);
        $rrdtool_graph['vertical-label'] = strlen($temp_vlabel) 
                   ?  $temp_vlabel
                   :  ' ';
    } else {
        $rrdtool_graph['vertical-label'] = ' ';
    }
 /*   else if ( isset($rrdtool_graph['lower-limit']) or 
              isset($rrdtool_graph['upper-limit'])    ) {
        $max = $max > 1000
                    ? number_format($max)
                    : number_format($max,2);
        $min = $min = 0
                    ? $min
                    : number_format($min,2);
        $rrdtool_graph['vertical-label'] = "'$min - $max'";    
    }
    else {
        $rrdtool_graph['vertical-label'] = "";    
    }
  */ 

    //# the actual graph...
    $series  = "DEF:'sum'='$rrd_dir/$metricname.rrd:sum':AVERAGE ";
    $series .= "AREA:'sum'#$default_metric_color:'$subtitle' ";

    if ($jobstart) {
        $series .= "VRULE:$jobstart#$jobstart_color ";
    }    
   
    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
