<?php
/* $Id$ */
include_once "./eval_conf.php";
# ATD - function.php must be included before get_context.php.  It defines some needed functions.
include_once "./functions.php";
include_once "./get_context.php";
include_once "./ganglia.php";
include_once "./get_ganglia.php";
include_once "./dwoo/dwooAutoload.php";

$resource = GangliaAcl::ALL_CLUSTERS;
if( $context == "grid" ) {
  $resource = $grid;
} else if ( $context == "cluster" || $context == "host" ) {
  $resource = $clustername; 
}
if( ! checkAccess( $resource, GangliaAcl::VIEW, $conf ) ) {
  header( "HTTP/1.1 403 Access Denied" );
  die("<html><head><title>Access Denied</title><body><h4>Sorry, you do not have access to this resource.</h4></body></html>");
}


try
   {
      $dwoo = new Dwoo($conf['dwoo_compiled_dir']);
   }
catch (Exception $e)
   {
   print "<H4>There was an error initializing the Dwoo PHP Templating Engine: ".
      $e->getMessage() . "<br><br>The compile directory should be owned and writable by the apache user.</H4>";
      exit;
   }

# Useful for addons.
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
      if (preg_match('/cluster/i', $clustername))
         $title = "$clustername Report";
      else
         $title = "$clustername Cluster Report";

      include_once "./header.php";
      include_once "./cluster_view.php";
   }
else if ($context == "physical")
   {
      $title = "$clustername Physical View";
      include_once "./header.php";
      include_once "./physical_view.php";
   }
else if ($context == "node")
   {
      $title = "$hostname Node View";
      include_once "./header.php";
      include_once "./show_node.php";
   }
else if ($context == "host")
   {
      $title = "$hostname Host Report";
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
