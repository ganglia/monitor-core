<?php
# $Id$
#
# read and evaluate the configuration file
#
include_once "./conf.php";

# These are settings derived from the configuration settings, and
# should not be modified.  This file will be overwritten on package upgrades,
# while changes made in conf.php should be preserved

$rrd_options = "";
if( isset( $rrdcached_socket ) )
{
    if(!empty( $rrdcached_socket ) )
    {
        $rrd_options .= " --daemon $rrdcached_socket";
    }
}



?>
