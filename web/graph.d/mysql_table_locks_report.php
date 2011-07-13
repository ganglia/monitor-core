<?php

/* Pass in by reference! */
function graph_mysql_table_locks_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Table Locks';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'locks_immediate'='${rrd_dir}/mysql_table_locks_immediate.rrd':'sum':AVERAGE "
		."DEF:'locks_waited'='${rrd_dir}/mysql_table_locks_waited.rrd':'sum':AVERAGE "
		."DEF:'slow_queries'='${rrd_dir}/mysql_slow_queries.rrd':'sum':AVERAGE "
		."AREA:'locks_immediate'#D2D8F9:'Locks Immediate' "
		."STACK:'locks_waited'#FF3932:'Locks Waited' "
		."STACK:'slow_queries'#35962B:'Slow Queries' "
		."LINE:'locks_immediate'#002A8F66:'' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

