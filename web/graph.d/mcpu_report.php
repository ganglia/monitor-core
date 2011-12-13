<?php

/* This report should work for either of:
     - modmulticpu for Linux (included in the core Ganglia distribution)
     - modmulticpu for Solaris (from the ganglia-modules-solaris,
            an independent project for Solaris modules)
 */

/* Pass in by reference! */
function graph_mcpu_report ( &$rrdtool_graph ) {

    global $context,
           $hostname,
           $range,
           $rrd_dir,
           $size,
           $strip_domainname;

    $mcpu_colors = array(0 => "5555cc", 1 => "0000aa", 2 => "33cc33", 
       3 => "99ff33", 4 => "00ff00", 5 => "9900CC");


    if ($strip_domainname) {
       $hostname = strip_domainname($hostname);
    }

    $rrdtool_graph['height'] += ($size == 'medium') ? 14 : 0;
    $title = 'All cores - utilization';
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['upper-limit']    = '100';
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Percent';
    $rrdtool_graph['extras']         = '--rigid';

    $series = "";
    $series_g = "";
    $dir_content = scandir($rrd_dir);
    foreach ($dir_content as $key => $content) {
        if(preg_match("/^multicpu_idle([0-9]+).rrd$/",
                $content, $matches) != 0) {
            $cpu_num = $matches[1];
            $col_num = $cpu_num % count($mcpu_colors);
            $mcpu_col = $mcpu_colors[$col_num];
            $series = $series."DEF:'idle${cpu_num}'='${rrd_dir}/${content}':'sum':AVERAGE "
                           . "CDEF:'util${cpu_num}'=100,idle${cpu_num},- ";
            $series_g = $series_g."LINE1:'util${cpu_num}'#$mcpu_col:'CPU ${cpu_num}' ";
        } 
    }

    $series = $series . $series_g;


    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;
}

?>
