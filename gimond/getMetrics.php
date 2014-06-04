<?php
   $fp = fsockopen('0.0.0.0', 8652);
   $recv = "";
   fwrite($fp, "/status\r\n");
   while (!feof($fp)) {
      $recv .= fgets($fp, 4096);
   }
   fclose($fp);
   //skip HTTP/1.0 200 OK Server: (...) Connection: close
   strtok($recv, "\r\n");
   for ($x=0; $x<3; $x++) {
      strtok("\r\n");
   }
   echo json_encode(strtok("\r\n"));
?>