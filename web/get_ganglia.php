<?php
# $Id$
# Retrieves and parses the XML output from gmond. Results stored
# in global variables: $clusters, $hosts, $hosts_down, $metrics.
# Assumes you have already called get_context.php.
#

if (! Gmetad($ganglia_ip, $ganglia_port) )
   {
      print "<H4>There was an error collecting ganglia data ".
         "($ganglia_ip:$ganglia_port): $error</H4>\n";
      exit;
   }

# If we have no child data sources, assume something is wrong.
if (!count($grid) and !count($cluster))
   {
      print "<H4>Ganglia cannot find a data source. Is gmond running?</H4>";
      exit;
   }

# If we only have one cluster source, suppress MetaCluster output.
if (count($grid) == 2 and $context=="meta")
   {
      # Lets look for one cluster (the other is our grid).
      foreach($grid as $source)
         if ($source[CLUSTER])
            {
               $standalone = 1;
               $context = "cluster";
               # Need to refresh data with new context.
               Gmetad($ganglia_ip, $ganglia_port);
               $clustername = $source[NAME];
            }
   }

?>
