<?php
/* $Id$ */
$tpl = new Dwoo_Template_File( template("footer.tpl") );
$data = new Dwoo_Data(); 
$data->assign("webfrontend_version",$version["webfrontend"]);

if ($version["rrdtool"]) {
   $data->assign("rrdtool_version",$version["rrdtool"]);
}

$backend_components = array("gmetad", "gmetad-python", "gmond");

foreach ($backend_components as $backend) {
   if (isset($version[$backend])) {
      $data->assign("webbackend_component", $backend);
      $data->assign("webbackend_version",$version[$backend]);
      break;
   }
}

$data->assign("parsetime", sprintf("%.4f", $parsetime) . "s");

$dwoo->output($tpl, $data);
?>
