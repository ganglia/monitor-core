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
    $rrdtool_graph['height'] += ($size != 'small') ? 14 : 0;
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Packets/sec';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

    $width= $size == 'small' ? 1 : 2;

    $fmt = '%5.1lf';
    $series = "'DEF:bytes_in=${rrd_dir}/pkts_in.rrd:sum:AVERAGE' "
       . "'DEF:bytes_out=${rrd_dir}/pkts_out.rrd:sum:AVERAGE' "
       . "'LINE$width:bytes_in#$mem_cached_color:In ' "
       . "'GPRINT:bytes_in:MIN:(Min\:$fmt%s' "
       . "'GPRINT:bytes_in:AVERAGE:Avg\:$fmt%s' "
       . "'GPRINT:bytes_in:MAX:Max\:$fmt%s)\\l' "
       . "'LINE$width:bytes_out#$mem_used_color:Out' "
       . "'GPRINT:bytes_out:MIN:(Min\:$fmt%s' "
       . "'GPRINT:bytes_out:AVERAGE:Avg\:$fmt%s' "
       . "'GPRINT:bytes_out:MAX:Max\:$fmt%s)\\l' ";

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
