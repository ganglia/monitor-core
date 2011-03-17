<?php
# $Id$
#
# read and evaluate the configuration file
#

# Load main config file.
require_once "./conf_default.php";

# Include user-defined overrides if they exist.
if( file_exists( "./conf.php" ) ) {
  include_once "./conf.php";
}

# These are settings derived from the configuration settings, and
# should not be modified.  This file will be overwritten on package upgrades,
# while changes made in conf.php should be preserved
$rrd_options = "";
if( isset( $conf['rrdcached_socket'] ) )
{
    if(!empty( $conf['rrdcached_socket'] ) )
    {
        $rrd_options .= " --daemon ${conf['rrdcached_socket']}";
    }
}



?>
