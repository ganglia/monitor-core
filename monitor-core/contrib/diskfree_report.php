<?php

/* Report to predict when the overall diskfree figures will reach zero */

/* Place this in the graph.d/ directory, and add "diskfree" to the
   $optional_graphs array in web/conf.php */

function graph_diskfree_report ( &$rrdtool_graph ) {

/* this is just the cpu_report (from revision r920) as an example, but with extra comments */

    // pull in a number of global variables, many set in conf.php (such as colors and $rrd_dir),
    // but other from elsewhere, such as get_context.php

    global $context, 
           $cpu_idle_color,
           $cpu_nice_color, 
           $cpu_system_color, 
           $cpu_user_color,
           $cpu_wio_color,
           $hostname,
           $range,
           $rrd_dir,
           $size;
    //
    // You *MUST* set at least the 'title', 'vertical-label', and 'series' variables.
    // Otherwise, the graph *will not work*.  
    //
    $rrdtool_graph['title']  = 'Diskfree Report';
                             //  This will be turned into:   
                             //  "Clustername $TITLE last $timerange", so keep it short
    $rrdtool_graph['vertical-label'] = 'Percent Free Space';
    $rrdtool_graph['upper-limit']    = '100';
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['extras']         = '--rigid';

    switch ($range) {

    case "day"   :
        $rrdtool_graph['end'] =  '+6h';
        break;
    case "week"  :
        $rrdtool_graph['end'] =  '+2d';
        break;
    case "month" :
        $rrdtool_graph['end'] =  '+3w';
        break;
    case "year"  :
        $rrdtool_graph['end'] =  '+2m';
        break;
    default:
        $rrdtool_graph['end'] =  'N';
        break;
   }


    /* Here we actually build the chart series.  This is moderately complicated
       to show off what you can do.  For a simpler example, look at network_report.php */
    if($context != "host" ) {

    }
    // Context is not "host"
    else {

    }

    $series_a = array(
        "DEF:'total_free'=$rrd_dir/disk_free.rrd:sum:AVERAGE",
        "DEF:'num_nodes'=$rrd_dir/disk_free.rrd:num:AVERAGE",
        "CDEF:free=total_free,num_nodes,/",
        "CDEF:threshold=num_nodes,UN,0,0,IF,5,+",
        "VDEF:intercept=free,LSLINT",
        "VDEF:slope=free,LSLSLOPE",
        "VDEF:correlation=free,LSLCORREL",
        "CDEF:free_trend=free,POP,intercept,COUNT,slope,*,+",
        "CDEF:space_exceeded=free_trend,threshold,LE,1,UNKN,IF",
        "VDEF:space_date=space_exceeded,FIRST",

        "AREA:free#999999",
        "LINE2:threshold#ff0000",
        "LINE1:free#000000:'Free Disk space\l'",
        "LINE2:free_trend#0000FF:'Current usage trend'",
        "VRULE:space_date#f00",
        "GPRINT:correlation:'Correlation is %1.2lf\l'",
        "GPRINT:space_date:'  Estimated out-of-space date is %a %d %B %Y\l':strftime",
    );

    $series = implode(' ',$series_a);

    // We have everything now, so add it to the array, and go on our way.
    $rrdtool_graph['series'] = $series;

//    error_log(var_export($rrdtool_graph,TRUE));

    return $rrdtool_graph;
}

?>
