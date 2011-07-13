<?php

/* Pass in by reference! */
function graph_mysql_qcache_hitrate_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Query Cache Hitrate';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['vertical-label'] = 'percent';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'hits'='${rrd_dir}/mysql_qcache_hits.rrd':'sum':AVERAGE "
		."DEF:'select'='${rrd_dir}/mysql_com_select.rrd':'sum':AVERAGE "
		."CDEF:'hitrate'=hits,hits,select,+,/,100,* "
		."AREA:'hitrate'#000098:'Hitrate' "
		."LINE:'hitrate'#7CB3F199:'' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

