<?php
/*
 *      xen_mem_report.php
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
function graph_xen_mem_report ( &$rrdtool_graph ) {

    global $context, 
           $hostname, 
           $mem_shared_color,
           $mem_cached_color, 
           $mem_buffered_color, 
           $mem_swapped_color, 
           $mem_used_color,
           $cpu_num_color,
           $range,
           $rrd_dir,
           $size;

    $title = 'Xen Memory'; 
    if ($context != 'host') {
       $rrdtool_graph['title'] = $title;
    } else {
       $rrdtool_graph['title'] = "$hostname $title last $range";
    }
    $rrdtool_graph['lower-limit']    = '0';
    $rrdtool_graph['vertical-label'] = 'Bytes';
    $rrdtool_graph['extras']         = '--rigid --base 1024';

     $series = "DEF:'xen_mem'='${rrd_dir}/xen_mem.rrd':'sum':AVERAGE "
        ."CDEF:'bxen_mem'=xen_mem,1024,* "

        ."DEF:'xen_mem_use'='${rrd_dir}/xen_mem_use.rrd':'sum':AVERAGE "
        ."CDEF:'bxen_mem_use'=xen_mem_use,1024,* "

        ."CDEF:'bxen_mem_free'='bxen_mem','xen_mem_use',- "

        ."AREA:'bxen_mem_use'#$mem_used_color:'Memory Used' ";
        $series .= "LINE2:'bxen_mem'#$cpu_num_color:'Total In-Core Memory' ";

 
    $rrdtool_graph['series'] = $series;

    return $rrdtool_graph;

}

?>
