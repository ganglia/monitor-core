<?php

/* Pass in by reference! */
function graph_mysql_innodb_bp_activity_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql InnoDB Buffer Pool Activity';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'created'='${rrd_dir}/mysql_innodb_pages_created.rrd':'sum':AVERAGE "
		."DEF:'read'='${rrd_dir}/mysql_innodb_pages_read.rrd':'sum':AVERAGE "
		."DEF:'written'='${rrd_dir}/mysql_innodb_pages_written.rrd':'sum':AVERAGE "
		."AREA:'created'#FFAB00:'Pages Created' "
		."STACK:'read'#D8ACE0:'Pages Read' "
		."STACK:'written'#7CB3F1:'Pages Written' "
		."LINE:'0'#5291D3:'':STACK "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

