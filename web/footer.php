<?php
include_once "conf.php";

/* $Id$ */
$tpl = new TemplatePower( template("footer.tpl") );
$tpl->prepare();

$tpl->assign("ganglia_version",$ganglia_version);
$tpl->assign("ganglia_release_name", $ganglia_release_name);
$tpl->assign("parsetime", sprintf("%.4f", $parsetime) . "s");

$tpl->printToScreen();
?>
