<?php
# $Id$
#
# read and evaluate the configuration file
#

$base_dir = dirname(__FILE__);
set_include_path( "$base_dir/lib:" . ini_get( 'include_path' ) );

# Load main config file.
require_once "./conf_default.php";
require_once 'lib/GangliaAcl.php';
require_once 'lib/GangliaAuth.php';

# Include user-defined overrides if they exist.
if( file_exists( "./conf.php" ) ) {
  include_once "./conf.php";
}

$errors = array();
// Installation validity checks
if ( ! isset($conf['rrds']) ||  ! is_readable($conf['rrds']) ) {
  $errors[] = "RRDs directory is not readable.<br/>".
  "Please adjust <code>\$conf['rrds']</code>."; 
}

if ( ! isset($conf['dwoo_compiled_dir']) || ! is_writeable($conf['dwoo_compiled_dir']) ) {
  $errors[] = "DWOO compiled templates directory is not writeable.<br/>".
  "Please adjust <code>\$conf['dwoo_compiled_dir']</code>."; 
}

if( ! isSet($conf['views_dir']) || ! is_readable($conf['views_dir']) ) {
  $errors[] = "Views directory is not readable.<br/>".
  "Please adjust <code>\$conf['views_dir']</code>.";
}

if( ! isSet($conf['conf_dir']) || ! is_readable($conf['conf_dir']) ) {
  $errors[] = "Directory used to store configuration information is not readable.<br/>".
  "Please adjust <code>\$conf['conf_dir']</code>.";
}

$valid_auth_options = array( 'disabled', 'readonly', 'enabled' );
if( ! isSet( $conf['auth_system'] ) ) {
  $errors[] = "Please define \$conf['auth_system'] and set it to one of these values:
  <ul>
    <li><code>'readonly'</code> : All users can view.  No-one can edit.</li>
    <li><code>'disabled'</code> : All users can view and edit anything.</li>
    <li><code>'enabled'</code> : All users can view public clusters.  
      Users may authenticate to gain additional access. (View private clusters, edit views, etc.)  
      Requires configuration of an authentication mechanism in your web server.
    </li>
  </ul>
  <br/>See <a href='https://sourceforge.net/apps/trac/ganglia/wiki/ganglia-web-2/AuthSystem'>https://sourceforge.net/apps/trac/ganglia/wiki/ganglia-web-2/AuthSystem</a> for more information.";
} else {
  if( ! in_array( $conf['auth_system'], $valid_auth_options ) ) {
    $errors[] = "Please set \$conf['auth_system'] to one of these values: '".implode( "','", $valid_auth_options ) ."'";
  } else if( $conf['auth_system'] == 'enabled' ) {    
    $auth = GangliaAuth::getInstance();
    if(!$auth->environmentIsValid()) {
      $errors[] = "Problems found with authorization system configuration:".
      "<ul><li>".implode("</li><li>",$auth->getEnvironmentErrors())."</li></ul>".
      "<br/>You may also use <code>\$conf['auth_system'] = 'readonly';</code> or <code>\$conf['auth_system'] = 'disabled';</code>";
    }
  }
}

if( count($errors) ) {
  echo "<h1>Errors were detected in your configuration.</h1>";
  echo "<ul class='errors'>";
  foreach($errors as $error) {
    echo "<li>$error</li>";
  }
  echo "</ul>";
  die();
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
