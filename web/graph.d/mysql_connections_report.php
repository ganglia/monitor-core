<?php

/* Pass in by reference! */
function graph_mysql_connections_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Connections';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'max_used_connections'='${rrd_dir}/mysql_max_used_connections.rrd':'sum':AVERAGE "
		."DEF:'max_connections'='${rrd_dir}/mysql_max_connections.rrd':'sum':AVERAGE "
		."DEF:'aborted_clients'='${rrd_dir}/mysql_aborted_clients.rrd':'sum':AVERAGE "
		."DEF:'aborted_connects'='${rrd_dir}/mysql_aborted_connects.rrd':'sum':AVERAGE "
		."DEF:'threads_connected'='${rrd_dir}/mysql_threads_connected.rrd':'sum':AVERAGE "
		."DEF:'connections'='${rrd_dir}/mysql_connections.rrd':'sum':AVERAGE "
		."AREA:'max_connections'#C0C0C0:'Max Connections' "
		."LINE:'max_connections'#00000033:'' "
		."AREA:'max_used_connections'#FFD660:'Max Used Connections' "
		."LINE:'max_used_connections'#00000033:'' "
		."LINE2:'aborted_clients'#FF3932:'Aborted Clients' "
		."LINE2:'aborted_connects'#00FF00:'Aborted Connects' "
		."LINE2:'threads_connected'#FF7D00:'Threads Connected' "
		."LINE2:'connections'#4444FF:'New Connections' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

