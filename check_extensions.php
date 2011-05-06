<?php

/*
 * Check if a list of PHP extensions have been loaded and if so
 * push them into the array $loaded_extensions
 */

$extensions_to_check = array('rrd');
$loaded_extensions = array();

foreach ($extensions_to_check as $e)
  if (extension_loaded($e))
    array_push($loaded_extensions, $e);

?>
