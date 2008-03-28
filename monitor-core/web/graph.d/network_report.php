<?php


/* Pass in by reference! */
function graph_network_report ( &$rrdtool_graph ) {

    global $context, 
           $hostname,
           $mem_cached_color, 
           $mem_used_color,
           $cpu_num_color,
           $rrd_dir,
           $size;
    
    $rrdtool_graph['height']        += $size == 'medium' ? 28 : 0 ;
    $rrdtool_graph['title']          = 'Network';
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Bytes/sec';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

    $series = "DEF:'bytes_in'='${rrd_dir}/bytes_in.rrd':'sum':AVERAGE "
       ."DEF:'bytes_out'='${rrd_dir}/bytes_out.rrd':'sum':AVERAGE "
       ."LINE2:'bytes_in'#$mem_cached_color:'In' "
       ."LINE2:'bytes_out'#$mem_used_color:'Out' ";

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
