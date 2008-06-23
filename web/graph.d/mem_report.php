<?php


/* Pass in by reference! */
function graph_mem_report ( &$rrdtool_graph ) {

    global $context, 
           $hostname, 
           $mem_shared_color,
           $mem_cached_color, 
           $mem_buffered_color, 
           $mem_swapped_color, 
           $mem_used_color,
           $cpu_num_color,
           $range,
           $rrd_dir,
           $size;

    $title = 'Memory'; 
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Bytes';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

    $series = "DEF:'mem_total'='${rrd_dir}/mem_total.rrd':'sum':AVERAGE "
        ."CDEF:'bmem_total'=mem_total,1024,* "
        ."DEF:'mem_shared'='${rrd_dir}/mem_shared.rrd':'sum':AVERAGE "
        ."CDEF:'bmem_shared'=mem_shared,1024,* "
        ."DEF:'mem_free'='${rrd_dir}/mem_free.rrd':'sum':AVERAGE "
        ."CDEF:'bmem_free'=mem_free,1024,* "
        ."DEF:'mem_cached'='${rrd_dir}/mem_cached.rrd':'sum':AVERAGE "
        ."CDEF:'bmem_cached'=mem_cached,1024,* "
        ."DEF:'mem_buffers'='${rrd_dir}/mem_buffers.rrd':'sum':AVERAGE "
        ."CDEF:'bmem_buffers'=mem_buffers,1024,* "
        ."CDEF:'bmem_used'='bmem_total','bmem_shared',-,'bmem_free',-,'bmem_cached',-,'bmem_buffers',- "
        ."AREA:'bmem_used'#$mem_used_color:'Memory Used' "
        ."STACK:'bmem_shared'#$mem_shared_color:'Memory Shared' "
        ."STACK:'bmem_cached'#$mem_cached_color:'Memory Cached' "
        ."STACK:'bmem_buffers'#$mem_buffered_color:'Memory Buffered' ";
    if (file_exists("$rrd_dir/swap_total.rrd")) {
        $series .= "DEF:'swap_total'='${rrd_dir}/swap_total.rrd':'sum':AVERAGE "
            ."DEF:'swap_free'='${rrd_dir}/swap_free.rrd':'sum':AVERAGE "
            ."CDEF:'bmem_swapped'='swap_total','swap_free',-,1024,* "
            ."STACK:'bmem_swapped'#$mem_swapped_color:'Memory Swapped' ";
    }
    $series .= "LINE2:'bmem_total'#$cpu_num_color:'Total In-Core Memory' ";

  
    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
