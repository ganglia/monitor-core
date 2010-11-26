<?php

include_once("./conf.php");

$debug = 0;

if(CACHEDATA == 1 && file_exists(CACHEFILE)){
        // check for the cached file
        // snag it and return it if it is still fresh
        $time_diff = time() - filemtime(CACHEFILE);
        $expires_in = CACHETIME - $time_diff;
        if( $time_diff < CACHETIME){
                if ( $debug == 1 ) {
                  echo("DEBUG: Fetching data from cache. Expires in " . $expires_in . " seconds.\n");
                }
                $index_array = unserialize(file_get_contents(CACHEFILE));
        }
}

if ( ! isset($index_array) ) {

  if ( $debug == 1 ) {
                  echo("DEBUG: Querying GMond for new data\n");
  }
  include_once $GLOBALS['ganglia_dir'] . "/conf.php";
  # Set up for cluster summary
  $context = "index_array";
  include_once $GLOBALS['ganglia_dir'] . "/functions.php";
  include_once $GLOBALS['ganglia_dir'] . "/ganglia.php";
  include_once $GLOBALS['ganglia_dir'] . "/get_ganglia.php";

  foreach ( $index_array['cluster'] as $hostname => $elements ) {
    $hosts[] = $hostname;
  }

  asort($hosts);
  $index_array['hosts'] = $hosts;

  file_put_contents(CACHEFILE, serialize($index_array));

}

?>
