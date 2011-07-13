<?php

/* Pass in by reference! */
function graph_mysql_select_types_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Select Types';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'full_join'='${rrd_dir}/mysql_select_full_join.rrd':'sum':AVERAGE "
		."DEF:'full_range'='${rrd_dir}/mysql_select_full_range_join.rrd':'sum':AVERAGE "
		."DEF:'range'='${rrd_dir}/mysql_select_range.rrd':'sum':AVERAGE "
		."DEF:'range_check'='${rrd_dir}/mysql_select_range_check.rrd':'sum':AVERAGE "
		."DEF:'scan'='${rrd_dir}/mysql_select_scan.rrd':'sum':AVERAGE "
		."AREA:'full_join'#FF0000:'Full Join' "
		."STACK:'full_range'#FF7D00:'Full Range' "
		."STACK:'range'#FFF200:'Range' "
		."STACK:'range_check'#00CF00:'Range Check' "
		."STACK:'scan'#7CB3F1:'Scan' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

