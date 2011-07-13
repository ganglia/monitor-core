<?php

/* Pass in by reference! */
function graph_mysql_traffic_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Network Traffic';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['vertical-label'] = 'Bytes';
	$rrdtool_graph['extras']         = '--rigid --base 1024';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'sent'='${rrd_dir}/mysql_bytes_sent.rrd':'sum':AVERAGE "
		."DEF:'received'='${rrd_dir}/mysql_bytes_received.rrd':'sum':AVERAGE "
		."AREA:'sent'#980000:'Sent' "
		."AREA:'received'#000098:'Received' "
		."LINE1:'0'#00000066:'' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

