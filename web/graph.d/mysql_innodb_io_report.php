<?php

/* Pass in by reference! */
function graph_mysql_innodb_io_report ( &$rrdtool_graph ) {

	global $context,
		$hostname,
		$range,
		$rrd_dir,
		$size,
		$strip_domainname;

	if ($strip_domainname) {
		$hostname = strip_domainname($hostname);
	}

	$title = 'Mysql InnoDB I/O';
	if ($context != 'host') {
		$rrdtool_graph['title'] = $title;
	} else {
		$rrdtool_graph['title'] = "$hostname $title last $range";
	}

	$rrdtool_graph['lower-limit']    = '0';
	$rrdtool_graph['extras']         = '--rigid';
	$rrdtool_graph['height'] += ($size == 'medium') ? 28 : 0;

	$series = "DEF:'file_reads'='${rrd_dir}/mysql_innodb_data_reads.rrd':'sum':AVERAGE "
		."DEF:'file_writes'='${rrd_dir}/mysql_innodb_data_writes.rrd':'sum':AVERAGE "
		."DEF:'file_syncs'='${rrd_dir}/mysql_innodb_data_fsyncs.rrd':'sum':AVERAGE "
		."DEF:'log_writes'='${rrd_dir}/mysql_innodb_log_writes.rrd':'sum':AVERAGE "
		."LINE2:'file_reads'#ED7600:'File Reads' "
		."LINE2:'file_writes'#157419:'File Writes' "
		."LINE2:'file_syncs'#4444FF:'File Syncs' "
		."LINE2:'log_writes'#DA4725:'Log Writes' "
		;

	$rrdtool_graph['series'] = $series;

	return $rrdtool_graph;
}

