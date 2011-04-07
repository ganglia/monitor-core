<?php

/* -------------------------------------------------------------------------
   This script generates rundeck.org compliant external resource model
   provider. It will generate a YAML file that lists all your nodes.
   Currently it dumps all known hosts.

   To enable it edit project.properties file for your project ie.

     $RUNDECK_HOME/projects/Default/etc/project.properties 

   add following lines

    project.resources.file = /opt/rundeck/projects/Default/etc/resources.yaml
    project.resources.url = http://ganglia.local/ganglia/api/rundeck.php?nodes=all&ver=1.0

   In Rundeck web UI there will be a link that says

   Update nodes for Project Default

   Rundeck will download the file and copy it to the file name specified
   in projects.resources.file. It will overwrite any contents so 
   be careful
--------------------------------------------------------------------------- */

header("Content-Type: text/plain");

$conf['ganglia_dir'] = dirname(dirname(__FILE__));

include_once $conf['ganglia_dir'] . "/eval_conf.php";
include_once $conf['ganglia_dir'] . "/functions.php";
include_once $conf['ganglia_dir'] . "/get_context.php";
$context = "cluster";
include_once $conf['ganglia_dir'] . "/ganglia.php";
include_once $conf['ganglia_dir'] . "/get_ganglia.php";

foreach ( $metrics as $node => $metric_array ) {

if ( isset($metric_array['os_name']['VAL']) ) {
print "$node:
  description: Rundeck node
  hostname: $node
  nodename: $node
  osArch: " . $metric_array['machine_type']['VAL'] . "
  osFamily: unix
  osName: " . $metric_array['os_name']['VAL'] . "
  osVersion: " . $metric_array['os_release']['VAL'] . "
  tags: ''
  username: root
";
}

}

?>