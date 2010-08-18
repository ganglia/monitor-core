<?php
# $Id$
#
# read and evaluate the configuration file
#
if ( file_exists("./conf.php") ) {
   include_once "./conf.php";
} else {
   $docroot = getcwd();
   print "<H4>$docroot/conf.php does not exist, did you forget to run 'make -C web install' in the ganglia source directory?</H4>\n";
   exit;
}

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
