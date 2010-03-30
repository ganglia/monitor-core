<?php

/* Pass in by reference! */
function graph_network_report ( &$rrdtool_graph )
{
    global $context,
           $hostname,
           $mem_cached_color,
           $mem_used_color,
           $cpu_num_color,
           $range,
           $rrd_dir,
           $size,
           $strip_domainname,
           $graphreport_stats;

    if ($strip_domainname) {
        $hostname = strip_domainname($hostname);
    }

    $title = 'Network';
    $rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;
    if( $graphreport_stats ) {
        $rrdtool_graph['height'] += ($size == 'medium') ? 52 : 0;
    }

    if ($context != 'host')  {
        $rrdtool_graph['title'] = $title;
    } else {
        $rrdtool_graph['title'] = "$hostname Network last $range";
    }
    
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Bytes/sec';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

    if ($size == 'small') {
        $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:7' : '';
        $eol1    = '\\l';
        $space1  = ' ';
        $space2  = '      ';
    } else if ($size == 'medium') {
        $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:7' : '';
        $eol1    = '';
        $space1  = ' ';
        $space2  = '';
    } else if ($size == 'large') {
        $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:10' : '';
        $eol1    = '';
        $space1  = '           ';
        $space2  = '     ';
    }

    $series = "DEF:'bytes_in'='${rrd_dir}/bytes_in.rrd':'sum':AVERAGE "
       ."DEF:'bytes_out'='${rrd_dir}/bytes_out.rrd':'sum':AVERAGE "
       ."LINE2:'bytes_in'#$mem_cached_color:'In\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:bytesin_pos=bytes_in,0,LT,0,bytes_in,IF "
                . "VDEF:bytesin_last=bytesin_pos,LAST "
                . "VDEF:bytesin_min=bytesin_pos,MINIMUM "
                . "VDEF:bytesin_avg=bytesin_pos,AVERAGE "
                . "VDEF:bytesin_max=bytesin_pos,MAXIMUM "
                . "GPRINT:'bytesin_last':' ${space1}Now\:%6.1lf%s' "
                . "GPRINT:'bytesin_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'bytesin_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'bytesin_max':'Max\:%6.1lf%s\\l' ";
    }

    $series .= "LINE2:'bytes_out'#$mem_used_color:'Out\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:bytesout_pos=bytes_out,0,LT,0,bytes_out,IF "
                . "VDEF:bytesout_last=bytesout_pos,LAST "
                . "VDEF:bytesout_min=bytesout_pos,MINIMUM "
                . "VDEF:bytesout_avg=bytesout_pos,AVERAGE "
                . "VDEF:bytesout_max=bytesout_pos,MAXIMUM " 
                . "GPRINT:'bytesout_last':'${space1}Now\:%6.1lf%s' "
                . "GPRINT:'bytesout_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'bytesout_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'bytesout_max':'Max\:%6.1lf%s\\l' ";
    }

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
