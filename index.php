<?php
/* $Id$ */
include_once "./conf.php";
include_once "./get_context.php";
include_once "./functions.php";
include_once "./ganglia.php";
include_once "./get_ganglia.php";
include_once "./class.TemplatePower.inc.php";
# Usefull for addons.
$GHOME = ".";

if ($context == "meta" or $context == "control")
   {
      $title = "$self $meta_designator Report";
      include_once "./header.php";
      include_once "./meta_view.php";
   }
else if ($context == "tree")
   {
      $title = "$self $meta_designator Tree";
      include_once "./header.php";
      include_once "./grid_tree.php";
   }
else if ($context == "cluster" or $context == "cluster-summary")
   {
      $title = "Cluster Report";
      include_once "./header.php";
      include_once "./cluster_view.php";
   }
else if ($context == "physical")
   {
      $title = "Physical View";
      include_once "./header.php";
      include_once "./physical_view.php";
   }
else if ($context == "node")
   {
      $title = "Node View";
      include_once "./header.php";
      include_once "./show_node.php";
   }
else if ($context == "host")
   {
      $title = "Host Report";
      include_once "./header.php";
      include_once "./host_view.php";
   }
else
   {
      $title = "Unknown Context";
      print "Unknown Context Error: Have you specified a host but not a cluster?.";
   }
include_once "./footer.php";

?>
