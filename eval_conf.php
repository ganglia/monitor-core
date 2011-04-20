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

// Installation validity checks
if ( ! isset($conf['rrds']) ||  ! is_readable($conf['rrds']) )
  die("<font color=red>ERROR: Directory with RRDs is inaccessible. Please make sure 
  you have an entry in conf.php that points to Ganglia RRDs and is
  readable by the Apache user e.g.<p>
    \$conf['rrds'] = '/var/lib/ganglia/rrds';</font>"); 

// 
if ( ! isset($conf['dwoo_compiled_dir']) || ! is_writeable($conf['dwoo_compiled_dir']) )
  die("<font color=red>ERROR: Directory used for DWOO compiled templates is not writeable. 
  Please make sure you have an entry in conf.php contains a directory that exists and 
  is writeable by the Apache user e.g.<p>
    \$conf['dwoo_compiled_dir'] = '/var/lib/ganglia/dwoo';</font>"); 

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
