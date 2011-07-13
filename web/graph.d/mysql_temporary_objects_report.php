<?php

/* Pass in by reference! */
function graph_mysql_temporary_objects_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql Temporary Objects';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'tables'='${rrd_dir}/mysql_created_tmp_tables.rrd':'sum':AVERAGE "
		."DEF:'disk_tables'='${rrd_dir}/mysql_created_tmp_disk_tables.rrd':'sum':AVERAGE "
		."DEF:'files'='${rrd_dir}/mysql_created_tmp_files.rrd':'sum':AVERAGE "
		."AREA:'tables'#FFAB00:'Temp Tables' "
		."LINE:'tables'#837C04:'' "
		."LINE:'disk_tables'#F51D30:'Temp Disk Tables' "
		."LINE:'files'#157419:'Temp Files' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

