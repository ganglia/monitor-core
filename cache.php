<?php

include_once("./eval_conf.php");

$debug = 0;

if($conf['cachedata'] == 1 && file_exists($conf['cachefile'])){
        // check for the cached file
        // snag it and return it if it is still fresh
        $time_diff = time() - filemtime($conf['cachefile']);
        $expires_in = $conf['cachetime'] - $time_diff;
        if( $time_diff < $conf['cachetime']){
                if ( $debug == 1 ) {
                  echo("DEBUG: Fetching data from cache. Expires in " . $expires_in . " seconds.\n");
                }
                $index_array = unserialize(file_get_contents($conf['cachefile']));
        }
}

if ( ! isset($index_array) ) {

  if ( $debug == 1 ) {
                  echo("DEBUG: Querying GMond for new data\n");
  }
  # Set up for cluster summary
  $context = "index_array";
  include_once $conf['ganglia_dir'] . "/functions.php";
  include_once $conf['ganglia_dir'] . "/ganglia.php";
  include_once $conf['ganglia_dir'] . "/get_ganglia.php";

  foreach ( $index_array['cluster'] as $hostname => $elements ) {
    $hosts[] = $hostname;
  }

  asort($hosts);
  $index_array['hosts'] = $hosts;

  file_put_contents($conf['cachefile'], serialize($index_array));

}

?>
