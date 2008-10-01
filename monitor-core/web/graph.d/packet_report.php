<?php

/* Pass in by reference! */
function graph_packet_report ( &$rrdtool_graph ) {

    global $context,
           $hostname,
           $mem_cached_color,
           $mem_used_color,
           $cpu_num_color,
           $range,
           $rrd_dir,
           $size,
           $strip_domainname;

    if ($strip_domainname) {
       $hostname = strip_domainname($hostname);
    }

    $title = 'Packets';
    $rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Packets/sec';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

    $series = "DEF:'bytes_in'='${rrd_dir}/pkts_in.rrd':'sum':AVERAGE "
       ."DEF:'bytes_out'='${rrd_dir}/pkts_out.rrd':'sum':AVERAGE "
       ."LINE2:'bytes_in'#$mem_cached_color:'In' "
       ."'GPRINT:bytes_in:MIN:(%.1lf %s Min' "
       ."'GPRINT:bytes_in:AVERAGE:%.1lf %s Avg' "
       ."'GPRINT:bytes_in:MAX:%.1lf %s Max)\\j' "
       ."LINE2:'bytes_out'#$mem_used_color:'Out' "
       ."'GPRINT:bytes_out:MIN:(%.1lf %s Min' "
       ."'GPRINT:bytes_out:AVERAGE:%.1lf %s Avg' "
       ."'GPRINT:bytes_out:MAX:%.1lf %s Max)\\j' ";

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
