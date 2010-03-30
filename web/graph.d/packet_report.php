<?php

/* Pass in by reference! */
function graph_packet_report ( &$rrdtool_graph )
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

    $title = 'Packets';
    $rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;
    if ( $graphreport_stats ) {
        $rrdtool_graph['height'] += ($size == 'medium') ? 52 : 0;
    }

    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Packets/sec';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

    if ($size == 'small') {
       $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:7' : '';
       $eol1    = '\\l';
       $space1  = ' ';
       $space2  = '      ';
    }
    if ($size == 'medium') {
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

    $series = "DEF:'pkts_in'='${rrd_dir}/pkts_in.rrd':'sum':AVERAGE "
       ."DEF:'pkts_out'='${rrd_dir}/pkts_out.rrd':'sum':AVERAGE "
       ."LINE2:'pkts_in'#$mem_cached_color:'In\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:pktsin_pos=pkts_in,0,LT,0,pkts_in,IF "
                . "VDEF:pktsin_last=pktsin_pos,LAST "
                . "VDEF:pktsin_min=pktsin_pos,MINIMUM "
                . "VDEF:pktsin_avg=pktsin_pos,AVERAGE "
                . "VDEF:pktsin_max=pktsin_pos,MAXIMUM "
                . "GPRINT:'pktsin_last':' ${space1}Now\:%6.1lf%s' "
                . "GPRINT:'pktsin_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'pktsin_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'pktsin_max':'Max\:%6.1lf%s\\l' ";
    }
    $series .= "LINE2:'pkts_out'#$mem_used_color:'Out\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:pktsout_pos=pkts_out,0,LT,0,pkts_out,IF "
                . "VDEF:pktsout_last=pktsout_pos,LAST "
                . "VDEF:pktsout_min=pktsout_pos,MINIMUM "
                . "VDEF:pktsout_avg=pktsout_pos,AVERAGE "
                . "VDEF:pktsout_max=pktsout_pos,MAXIMUM " 
                . "GPRINT:'pktsout_last':'${space1}Now\:%6.1lf%s' "
                . "GPRINT:'pktsout_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'pktsout_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'pktsout_max':'Max\:%6.1lf%s\\l' ";
    }

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
