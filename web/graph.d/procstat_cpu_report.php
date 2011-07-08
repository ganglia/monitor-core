<?php

/* Pass in by reference! */
function graph_procstat_cpu_report ( &$rrdtool_graph ) {

    global $context,
           $hostname,
           $graph_var,
           $range,
           $rrd_dir,
           $size,
           $strip_domainname;

$rainbow = array(
    '008000', // green
    '0000FF', // blue
    'FF0000', // red
    'FFEA00', // yellow
    '9900CC', // purple
    '00FF00', // lime green
    '00FFFF', // aqua blue
    'FF00FF', // magenta
    '808000', // olive
    '008080', // teal
    '808080', // gray
    '000000'  // black
);

    if ($strip_domainname) {
       $hostname = strip_domainname($hostname);
    }

    $title = 'Process CPU Usage';
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'percent';
	$rrdtool_graph['extras']         = '--rigid';
    $rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = '';
	foreach (explode(',', $graph_var) as $k => $proc) {
		$color = $rainbow[$k%count($rainbow)];

		$series .= "DEF:'${proc}'='${rrd_dir}/procstat_${proc}_cpu.rrd':'sum':AVERAGE "
		."LINE2:'${proc}'#${color}:'${proc}' ";
		//."LINE2:'${proc}'#555555:'${proc}' ";
	}

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
