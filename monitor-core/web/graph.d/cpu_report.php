<?php

/* Pass in by reference! */
function graph_cpu_report( &$rrdtool_graph ) {
    global $context,
           $cpu_idle_color,
           $cpu_nice_color,
           $cpu_system_color,
           $cpu_user_color,
           $cpu_wio_color,
           $hostname,
           $range,
           $rrd_dir,
           $size,
           $strip_domainname,
           $graphreport_stats;

    if ($strip_domainname)     {
       $hostname = strip_domainname($hostname);
    }

    $title = 'CPU';
    if ($context != 'host')     {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }

    $rrdtool_graph['upper-limit']    = '100';
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Percent';
    $rrdtool_graph['extras']         = '--rigid';
    $rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

    if( $graphreport_stats ) {
        $rrdtool_graph['height'] += ($size == 'medium') ? 16 : 0;
        $rmspace = '\\g';
    } else {
        $rmspace = '';
    }

    $series        = '';

    // RB: Perform some formatting/spacing magic.. tinkered to fit
    //
    if ($size == 'small') {
       $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:7' : '';
       $eol1        = '\\l';
       $space1  = ' ';
       $space2  = '         ';
    } else if ($size == 'medium') {
       $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:7' : '';
       $eol1        = '';
       $space1  = ' ';
       $space2  = '';
    } else if ($size == 'large') {
       $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:10' : '';
       $eol1        = '';
       $space1  = '           ';
       $space2  = '      ';
    }

    if ($context != "host" ) {
        $series .= "DEF:'num_nodes'='${rrd_dir}/cpu_user.rrd':'num':AVERAGE ";
    }
    $series .= "DEF:'cpu_user'='${rrd_dir}/cpu_user.rrd':'sum':AVERAGE "
            . "DEF:'cpu_nice'='${rrd_dir}/cpu_nice.rrd':'sum':AVERAGE "
            . "DEF:'cpu_system'='${rrd_dir}/cpu_system.rrd':'sum':AVERAGE "
            . "DEF:'cpu_idle'='${rrd_dir}/cpu_idle.rrd':'sum':AVERAGE ";

    if (file_exists("$rrd_dir/cpu_wio.rrd")) {
        $series .= "DEF:'cpu_wio'='${rrd_dir}/cpu_wio.rrd':'sum':AVERAGE ";
    }

    if ($context != "host" ) {
        $series .= "CDEF:'ccpu_user'=cpu_user,num_nodes,/ "
            . "CDEF:'ccpu_nice'=cpu_nice,num_nodes,/ "
            . "CDEF:'ccpu_system'=cpu_system,num_nodes,/ "
            . "CDEF:'ccpu_idle'=cpu_idle,num_nodes,/ ";

        if (file_exists("$rrd_dir/cpu_wio.rrd")) {
            $series .= "CDEF:'ccpu_wio'=cpu_wio,num_nodes,/ ";
        }

        $plot_prefix='ccpu';
    } else {
        $plot_prefix='cpu';
    }

    $series .= "AREA:'${plot_prefix}_user'#$cpu_user_color:'User${rmspace}' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:user_pos=${plot_prefix}_user,0,LT,0,${plot_prefix}_user,IF "
                . "VDEF:user_last=user_pos,LAST "
                . "VDEF:user_min=user_pos,MINIMUM "
                . "VDEF:user_avg=user_pos,AVERAGE "
                . "VDEF:user_max=user_pos,MAXIMUM "
                . "GPRINT:'user_last':'  ${space1}Now\:%5.1lf%%' "
                . "GPRINT:'user_min':'Min\:%5.1lf%%${eol1}' "
                . "GPRINT:'user_avg':'${space2}Avg\:%5.1lf%%' "
                . "GPRINT:'user_max':'Max\:%5.1lf%%\\l' ";
    }
    $series .= "STACK:'${plot_prefix}_nice'#$cpu_nice_color:'Nice${rmspace}' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:nice_pos=${plot_prefix}_nice,0,LT,0,${plot_prefix}_nice,IF " 
                . "VDEF:nice_last=nice_pos,LAST "
                . "VDEF:nice_min=nice_pos,MINIMUM "
                . "VDEF:nice_avg=nice_pos,AVERAGE "
                . "VDEF:nice_max=nice_pos,MAXIMUM "
                . "GPRINT:'nice_last':'  ${space1}Now\:%5.1lf%%' "
                . "GPRINT:'nice_min':'Min\:%5.1lf%%${eol1}' "
                . "GPRINT:'nice_avg':'${space2}Avg\:%5.1lf%%' "
                . "GPRINT:'nice_max':'Max\:%5.1lf%%\\l' ";
    }
    $series .= "STACK:'${plot_prefix}_system'#$cpu_system_color:'System${rmspace}' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:system_pos=${plot_prefix}_system,0,LT,0,${plot_prefix}_system,IF "
                . "VDEF:system_last=system_pos,LAST "
                . "VDEF:system_min=system_pos,MINIMUM "
                . "VDEF:system_avg=system_pos,AVERAGE "
                . "VDEF:system_max=system_pos,MAXIMUM "
                . "GPRINT:'system_last':'${space1}Now\:%5.1lf%%' "
                . "GPRINT:'system_min':'Min\:%5.1lf%%${eol1}' "
                . "GPRINT:'system_avg':'${space2}Avg\:%5.1lf%%' "
                . "GPRINT:'system_max':'Max\:%5.1lf%%\\l' ";
    }

    if (file_exists("$rrd_dir/cpu_wio.rrd")) {
        $series .= "STACK:'${plot_prefix}_wio'#$cpu_wio_color:'Wait${rmspace}' ";

        if ( $graphreport_stats ) {
                $series .= "CDEF:wio_pos=${plot_prefix}_wio,0,LT,0,${plot_prefix}_wio,IF "
                        . "VDEF:wio_last=wio_pos,LAST "
                        . "VDEF:wio_min=wio_pos,MINIMUM "
                        . "VDEF:wio_avg=wio_pos,AVERAGE "
                        . "VDEF:wio_max=wio_pos,MAXIMUM "
                            . "GPRINT:'wio_last':'  ${space1}Now\:%5.1lf%%' "
                        . "GPRINT:'wio_min':'Min\:%5.1lf%%${eol1}' "
                        . "GPRINT:'wio_avg':'${space2}Avg\:%5.1lf%%' "
                        . "GPRINT:'wio_max':'Max\:%5.1lf%%\\l' ";
        }
    }

    $series .= "STACK:'${plot_prefix}_idle'#$cpu_idle_color:'Idle${rmspace}' ";

    if ( $graphreport_stats ) {
                $series .= "CDEF:idle_pos=${plot_prefix}_idle,0,LT,0,${plot_prefix}_idle,IF "
                        . "VDEF:idle_last=idle_pos,LAST "
                        . "VDEF:idle_min=idle_pos,MINIMUM "
                        . "VDEF:idle_avg=idle_pos,AVERAGE "
                        . "VDEF:idle_max=idle_pos,MAXIMUM "
                        . "GPRINT:'idle_last':'  ${space1}Now\:%5.1lf%%' "
                        . "GPRINT:'idle_min':'Min\:%5.1lf%%${eol1}' "
                        . "GPRINT:'idle_avg':'${space2}Avg\:%5.1lf%%' "
                        . "GPRINT:'idle_max':'Max\:%5.1lf%%\\l' ";
    }

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;
}

?>
