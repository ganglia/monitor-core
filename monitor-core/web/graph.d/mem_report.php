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
           $size,
           $strip_domainname,
           $graphreport_stats;

    if ($strip_domainname) {
       $hostname = strip_domainname($hostname);
    }

    $title = 'Memory';

    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Bytes';
    $rrdtool_graph['extras']         = '--rigid --base 1024';
    $rrdtool_graph['height']        += ($size == 'medium') ? 28 : 0;

    if ( $graphreport_stats ) {
        $rrdtool_graph['height'] += ($size == 'medium') ? 4 : 0;
    }

    if ($size == 'small') {
       $rrdtool_graph['extras']        .= ($graphreport_stats == true) ? ' --font LEGEND:7' : '';
       $eol1    = '\\l';
       $space1  = ' ';
       $space2  = '         ';
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
        ."AREA:'bmem_used'#$mem_used_color:'Used\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:used_pos=bmem_used,0,LT,0,bmem_used,IF " 
                . "VDEF:used_last=used_pos,LAST "
                . "VDEF:used_min=used_pos,MINIMUM " 
                . "VDEF:used_avg=used_pos,AVERAGE " 
                . "VDEF:used_max=used_pos,MAXIMUM " 
                . "GPRINT:'used_last':'  ${space1}Now\:%6.1lf%s' "
                . "GPRINT:'used_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'used_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'used_max':'Max\:%6.1lf%s\\l' ";
    }
    $series .= "STACK:'bmem_shared'#$mem_shared_color:'Shared\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:shared_pos=bmem_shared,0,LT,0,bmem_shared,IF "
                . "VDEF:shared_last=shared_pos,LAST "
                . "VDEF:shared_min=shared_pos,MINIMUM " 
                . "VDEF:shared_avg=shared_pos,AVERAGE " 
                . "VDEF:shared_max=shared_pos,MAXIMUM " 
                . "GPRINT:'shared_last':'${space1}Now\:%6.1lf%s' "
                . "GPRINT:'shared_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'shared_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'shared_max':'Max\:%6.1lf%s\\l' ";
    }
    $series .= "STACK:'bmem_cached'#$mem_cached_color:'Cached\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:cached_pos=bmem_cached,0,LT,0,bmem_cached,IF "
                . "VDEF:cached_last=cached_pos,LAST "
                . "VDEF:cached_min=cached_pos,MINIMUM " 
                . "VDEF:cached_avg=cached_pos,AVERAGE " 
                . "VDEF:cached_max=cached_pos,MAXIMUM " 
                . "GPRINT:'cached_last':'${space1}Now\:%6.1lf%s' "
                . "GPRINT:'cached_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'cached_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'cached_max':'Max\:%6.1lf%s\\l' ";
    }
    $series .= "STACK:'bmem_buffers'#$mem_buffered_color:'Buffer\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:buffers_pos=bmem_buffers,0,LT,0,bmem_buffers,IF "
                . "VDEF:buffers_last=buffers_pos,LAST "
                . "VDEF:buffers_min=buffers_pos,MINIMUM " 
                . "VDEF:buffers_avg=buffers_pos,AVERAGE " 
                . "VDEF:buffers_max=buffers_pos,MAXIMUM " 
                . "GPRINT:'buffers_last':'${space1}Now\:%6.1lf%s' "
                . "GPRINT:'buffers_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'buffers_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'buffers_max':'Max\:%6.1lf%s\\l' ";
    }

    if (file_exists("$rrd_dir/swap_total.rrd")) {
        $series .= "DEF:'swap_total'='${rrd_dir}/swap_total.rrd':'sum':AVERAGE "
            ."DEF:'swap_free'='${rrd_dir}/swap_free.rrd':'sum':AVERAGE "
            ."CDEF:'bmem_swapped'='swap_total','swap_free',-,1024,* "
            ."STACK:'bmem_swapped'#$mem_swapped_color:'Swap\\g' ";

            if ( $graphreport_stats ) {
                $series .= "CDEF:swapped_pos=bmem_swapped,0,LT,0,bmem_swapped,IF "
                        . "VDEF:swapped_last=swapped_pos,LAST "
                        . "VDEF:swapped_min=swapped_pos,MINIMUM " 
                        . "VDEF:swapped_avg=swapped_pos,AVERAGE " 
                        . "VDEF:swapped_max=swapped_pos,MAXIMUM " 
                        . "GPRINT:'swapped_last':'  ${space1}Now\:%6.1lf%s' "
                        . "GPRINT:'swapped_min':'Min\:%6.1lf%s${eol1}' "
                        . "GPRINT:'swapped_avg':'${space2}Avg\:%6.1lf%s' "
                        . "GPRINT:'swapped_max':'Max\:%6.1lf%s\\l' ";
        }
    }

    $series .= "LINE2:'bmem_total'#$cpu_num_color:'Total\\g' ";

    if ( $graphreport_stats ) {
        $series .= "CDEF:total_pos=bmem_total,0,LT,0,bmem_total,IF "
                . "VDEF:total_last=total_pos,LAST "
                . "VDEF:total_min=total_pos,MINIMUM " 
                . "VDEF:total_avg=total_pos,AVERAGE " 
                . "VDEF:total_max=total_pos,MAXIMUM " 
                . "GPRINT:'total_last':' ${space1}Now\:%6.1lf%s' "
                . "GPRINT:'total_min':'Min\:%6.1lf%s${eol1}' "
                . "GPRINT:'total_avg':'${space2}Avg\:%6.1lf%s' "
                . "GPRINT:'total_max':'Max\:%6.1lf%s\\l' ";
    }

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
