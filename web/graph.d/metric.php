<?php

// This report is used for specific metric graphs at the bottom of the
// cluster_view page.

/* Pass in by reference! */
function graph_metric ( &$rrdtool_graph ) {

    global $context,
           $default_metric_color,
           $hostname,
           $jobstart,
           $jobstart_color,
           $load_color,
           $max,
           $meta_designator,
           $metricname,
           $metrictitle,
           $min,
           $range,
           $rrd_dir,
           $size,
           $summary,
           $value,
           $vlabel,
           $strip_domainname;

    if ($strip_domainname) {
        $hostname = strip_domainname($hostname);
    }

    $rrdtool_graph['height'] += 0; //no fudge needed

    switch ($context) {

        case 'host':

            if ($summary) {
                $rrdtool_graph['title'] = $hostname;
                $prefix = $metricname;
            } else {
                $prefix = $hostname;
                if ($metrictitle) {
                   $rrdtool_graph['title'] = $metrictitle;
                } else {
                   $rrdtool_graph['title'] = $metricname;
                }
            }

            $prefix = $summary ? $metricname : $hostname;
            $value = ($value > 1000)
                        ? number_format($value)
                        : number_format($value, 2);

            if ($range == 'job') {
                $hrs = intval (-$jobrange / 3600);
                $subtitle = "$prefix last ${hrs} (now $value)";
            } else {
                if ($summary) {
                   $subtitle_one = "$metricname last $range";
                } else {
                   $subtitle_one = "$hostname last $range";
                }
                $subtitle_two = "  (now $value)";
            }

            break;

        case 'meta':
            $rrdtool_graph['title'] = "$meta_designator ". $rrdtool_graph['title'] ."last $range";
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

    if ($vlabel) {
        // We should set $vlabel, even if it isn't used for spacing
        // and alignment reasons.  This is mostly for aesthetics
        $temp_vlabel = trim($vlabel);
        $rrdtool_graph['vertical-label'] = strlen($temp_vlabel)
                   ?  $temp_vlabel
                   :  ' ';
    } else {
        $rrdtool_graph['vertical-label'] = ' ';
    }

    // the actual graph...
    $series  = "'DEF:sum=$rrd_dir/$metricname.rrd:sum:AVERAGE' ";
    $series .= "'AREA:sum#$default_metric_color:$subtitle_one' ";
    $series .= "'VDEF:summax=sum,MAXIMUM' ";
    $series .= "'LINE1:summax#ff0000:$subtitle_one' ";
    $series .= "'COMMENT:$subtitle_two\\l' ";

    if ($jobstart) {
        $series .= " 'VRULE:$jobstart#$jobstart_color' ";
    }
    if ($size != "small") {
        $fmt = '%.1lf';
        $series .= "'GPRINT:sum:MIN:(Min\:$fmt%s' ";
        $series .= "'GPRINT:sum:AVERAGE:Avg\:$fmt%s' ";
        $series .= "'GPRINT:sum:MAX:Max\:$fmt%s)' ";
        $series .= "'GPRINT:sum:LAST:Last\:$fmt%s\\l' ";
    }
    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
