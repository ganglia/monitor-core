<?php
/*
 *      xen_vms_report.php
 *      
 *      Copyright 2011 Marcos Amorim <marcosmamorim@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


/* Pass in by reference! */
function graph_xen_vms_report ( &$rrdtool_graph ) {

    global $context, 
           $hostname,
           $mem_cached_color, 
           $mem_used_color,
           $cpu_num_color,
           $range,
           $rrd_dir,
           $size;
   
    $title = 'Virtual Machines'; 
    $rrdtool_graph['height']        += $size == 'medium' ? 28 : 0 ;
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Qtd';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

    $series = "DEF:'xen_vms'='${rrd_dir}/xen_vms.rrd':'sum':AVERAGE "
       ."LINE2:'xen_vms'#$mem_used_color:'VMS' ";

    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
