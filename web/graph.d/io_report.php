<?php

/* This has been tested with IO metrics from ganglia-modules-solaris
   It would also work with IO metrics from Linux or other platforms,
   once the necessary Ganglia module is developed and if the same
   naming convetions are used */

/* Pass in by reference! */
function graph_io_report ( &$rrdtool_graph ) {

    global $context,
           $hostname,
           $range,
           $rrd_dir,
           $size,
           $strip_domainname;

    /* Scale rate from left y-axis to right y-axis

       Should be set to a meaningful value for your site and IO levels
       you are studying.  E.g. 

         scalefactor = your max throughput / max(typical svc time in secs)

       We currently set this statically because RRDtool doesn't have a way
       to guess an appropriate value.

       It is further complicated by the fact that we could calculate the
       value with a VDEF, but rrdtool expects this parameter to be passed
       in a separate command line argument, so it can't be substituted
       by the VDEF */
    $scalefactor = 2000000000;

    $io_colours = array(0 => "5555cc", 1 => "0000aa", 2 => "33cc33", 
         3 => "99ff33", 4 => "00ff00", 5 => "9900CC");

    if ($strip_domainname) {
       $hostname = strip_domainname($hostname);
    }

    $rrdtool_graph['height'] += ($size == 'medium') ? 14 : 0;
    $title = 'IO avg svc time';
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    /* $rrdtool_graph['upper-limit']    = '100'; */
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 's';
    $rrdtool_graph['extras']         = '--rigid';

    $series = "";
    $series_datarate = "";
    $series_g = "";
    $dir_content = scandir($rrd_dir);
    $col_num = 0;
    foreach ($dir_content as $key => $content) {
        if(preg_match("/^io_avg_svc_time_([a-z0-9]+).rrd$/",
                $content, $matches) != 0) {
            $disk_name = $matches[1];
            if($col_num >= count($io_colours))
                $col_num = 0;
            $disk_col = $io_colours[$col_num];
            $series = $series."DEF:'avgsvct${disk_name}'='${rrd_dir}/${content}':'sum':AVERAGE "
                              ."DEF:'nread${disk_name}'='${rrd_dir}/io_nread_${disk_name}.rrd':'sum':AVERAGE "
                              ."DEF:'nwrite${disk_name}'='${rrd_dir}/io_nwrite_${disk_name}.rrd':'sum':AVERAGE "
                              ."CDEF:'datarate${disk_name}'=nread${disk_name},nwrite${disk_name},+ ";
            if($series_datarate == "") {
                $series_datarate = "CDEF:'aggdatarate'='datarate${disk_name}'";
            } else {
                $series_datarate = $series_datarate.",'datarate${disk_name}',+";
            }
            $series_g = $series_g."LINE1:'avgsvct${disk_name}'#$disk_col:'Disk ${disk_name}' ";

        } 
        $col_num++;
    }

    $series = $series 
        .$series_datarate." "
        ."CDEF:adjaggdatarate=aggdatarate,${scalefactor},/ "
        ."AREA:'adjaggdatarate'#e2e2f2:'Transfer rate' "
        . $series_g;

    $rrdtool_graph['right-axis-label'] = 'Bytes/sec';
    $rrdtool_graph['right-axis'] = $scalefactor.':0';

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;
}

?>
