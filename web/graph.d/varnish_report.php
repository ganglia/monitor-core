<?php


/* Pass in by reference! */
function graph_varnish_report ( &$rrdtool_graph ) {

   global $context, 
           $cpu_idle_color,
           $cpu_nice_color, 
           $cpu_system_color, 
           $cpu_user_color,
           $cpu_wio_color,
	   $mem_swapped_color,
           $hostname,
           $range,
           $rrd_dir,
           $size;

    $title = 'Varnish Report';
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = strip_domainname( $hostname ) ." $title last $range";

    }

    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'req_per_sec';
    $rrdtool_graph['extras']         = '--rigid';
    $rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

    if( $graphreport_stats ) {
        $rrdtool_graph['height'] += ($size == 'medium') ? 16 : 0;
        $rmspace = '\\g';
    } else {
        $rmspace = '';
    }
 
    if($context != "host" )
                {
                   /* If we are not in a host context, then we need to calculate the average */
                    $rrdtool_graph['series'] =
                   "DEF:'num_nodes'='${rrd_dir}/varnish_200.rrd':'num':AVERAGE "
                   ."DEF:'varnish_200'='${rrd_dir}/varnish_200.rrd':'sum':AVERAGE "
                   ."CDEF:'cvarnish_200'=varnish_200,num_nodes,/ "
                   ."DEF:'varnish_300'='${rrd_dir}/varnish_300.rrd':'sum':AVERAGE "
                   ."CDEF:'cvarnish_300'=varnish_300,num_nodes,/ "
                   ."DEF:'varnish_400'='${rrd_dir}/varnish_400.rrd':'sum':AVERAGE "
                   ."CDEF:'cvarnish_400'=varnish_400,num_nodes,/ "
                   ."DEF:'varnish_500'='${rrd_dir}/varnish_500.rrd':'sum':AVERAGE "
                   ."CDEF:'cvarnish_500'=varnish_500,num_nodes,/ "
                   ."DEF:'varnish_other'='${rrd_dir}/varnish_other.rrd':'sum':AVERAGE "
                   ."AREA:'varnish_200'#$cpu_user_color:'200' "
                   ."STACK:'varnish_300'#$cpu_nice_color:'300' "
                   ."STACK:'varnish_400'#$cpu_system_color:'400' "
                   ."STACK:'varnish_500'#$cpu_wio_color:'500' "
                   ."STACK:'varnish_other'#$cpu_idle_color:'other' "
		   ."LINE2:'varnish_unique_users'#$mem_swapped_color:'Unique IPs' ";

                }
     else
                {
                    $rrdtool_graph['series'] ="DEF:'varnish_200'='${rrd_dir}/varnish_200.rrd':'sum':AVERAGE "
                   ."DEF:'varnish_300'='${rrd_dir}/varnish_300.rrd':'sum':AVERAGE "
                   ."DEF:'varnish_400'='${rrd_dir}/varnish_400.rrd':'sum':AVERAGE "
                   ."DEF:'varnish_500'='${rrd_dir}/varnish_500.rrd':'sum':AVERAGE "
                   ."AREA:'varnish_200'#$cpu_user_color:'200' "
                   ."STACK:'varnish_300'#$cpu_nice_color:'300' "
                   ."STACK:'varnish_400'#$cpu_system_color:'400' "
                   ."STACK:'varnish_500'#$cpu_wio_color:'500' ";
                }

     #################################################################################
     # If there are no Apache metrics put something so that the report doesn't barf
     # I am using the CPU number metric since that one should always be there.
     #################################################################################
     if ( !file_exists("$rrd_dir/varnish_200.rrd")) {
		$rrdtool_graph['series'] =  "DEF:'cpu_num'='${rrd_dir}/cpu_num.rrd':'sum':AVERAGE "
		   ."LINE2:'cpu_num'#$mem_swapped_color:'Varnish metrics not collected' ";
     }

return $rrdtool_graph;
}


?>
