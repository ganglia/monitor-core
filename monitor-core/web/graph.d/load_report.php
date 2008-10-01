<?php

/* Pass in by reference! */
function graph_load_report ( &$rrdtool_graph ) {

    global $context,
           $cpu_num_color,
           $cpu_user_color,
           $hostname,
           $load_one_color,
           $num_nodes_color,
           $proc_run_color,
           $range,
           $rrd_dir,
           $size,
           $strip_domainname;

    if ($strip_domainname) {
       $hostname = strip_domainname($hostname);
    }

    $rrdtool_graph['height'] += ($size == 'medium') ? 14 : 0;
    $title = 'Load';
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Load/Procs';
    $rrdtool_graph['extras']         = '--rigid';

    $series = "DEF:'load_one'='${rrd_dir}/load_one.rrd':'sum':AVERAGE "
        ."DEF:'proc_run'='${rrd_dir}/proc_run.rrd':'sum':AVERAGE "
        ."DEF:'cpu_num'='${rrd_dir}/cpu_num.rrd':'sum':AVERAGE ";

    $series .="AREA:'load_one'#$load_one_color:'1-min Load' ";
    $series .="'GPRINT:load_one:AVERAGE:%.1lf' ";
    $series .="CDEF:util=load_one,cpu_num,/,100,* ";
    $series .="'GPRINT:util:AVERAGE:(%.1lf%%)' ";

    if( $context != 'host' ) {
        $series .="DEF:'num_nodes'='${rrd_dir}/cpu_num.rrd':'num':AVERAGE ";
        $series .= "LINE2:'num_nodes'#$num_nodes_color:'Nodes' ";
        $series .= "'GPRINT:num_nodes:AVERAGE:%.1lf' ";
        $proc_label = 'Running';
    } else {
        $proc_label = 'Running Processes';
    }

    $series .="LINE2:'cpu_num'#$cpu_num_color:'CPUs' ";
    $series .="'GPRINT:cpu_num:AVERAGE:%.1lf' ";
    $series .="LINE2:'proc_run'#$proc_run_color:'$proc_label' ";
    $series .="'GPRINT:proc_run:AVERAGE:%.1lf' ";
    $series .="CDEF:util2=proc_run,cpu_num,/,100,* ";
    $series .="'GPRINT:util2:AVERAGE:(%.1lf%%)' ";

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
