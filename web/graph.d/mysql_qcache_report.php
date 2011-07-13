<?php

/* Pass in by reference! */
function graph_mysql_qcache_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Query Cache';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'hits'='${rrd_dir}/mysql_qcache_hits.rrd':'sum':AVERAGE "
		."DEF:'inserts'='${rrd_dir}/mysql_qcache_inserts.rrd':'sum':AVERAGE "
		."DEF:'not_cached'='${rrd_dir}/mysql_qcache_not_cached.rrd':'sum':AVERAGE "
		."DEF:'lowmem_prunes'='${rrd_dir}/mysql_qcache_lowmem_prunes.rrd':'sum':AVERAGE "
		."AREA:'hits'#EAAF00:'Hits' "
		."STACK:'inserts'#157419:'Inserts' "
		."STACK:'not_cached'#00A0C1:'Not Cached' "
		."STACK:'lowmem_prunes'#FF0000:'Low-Memory Prunes' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

