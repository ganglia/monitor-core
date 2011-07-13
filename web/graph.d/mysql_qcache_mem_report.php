<?php

/* Pass in by reference! */
function graph_mysql_qcache_mem_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Query Cache Memory';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['vertical-label'] = 'Bytes';
	$rrdtool_graph['extras']         = '--rigid --base 1024';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'cache_size'='${rrd_dir}/mysql_query_cache_size.rrd':'sum':AVERAGE "
		."DEF:'free_mem'='${rrd_dir}/mysql_qcache_free_memory.rrd':'sum':AVERAGE "
		."CDEF:'_mem_used'=cache_size,free_mem,- "
		."AREA:'cache_size'#C0C0C0:'Cache Size' "
		."LINE:'cache_size'#00000033:'' "
		."AREA:'_mem_used'#FFD660:'Cache Used' "
		."LINE:'_mem_used'#00000033:'' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

