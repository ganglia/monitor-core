<!DOCTYPE html>
<html>
<body>
<h2>Testing metrics</h2>

<script>
function showMetrics(){
   var metrics = <?php
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
                  ?>;
   var pm = JSON.parse(metrics);
   for (var k in pm){
      var p = document.createElement('metrics');
      p.innerHTML = k + ': ' + pm[k]+'<br>';
      document.body.appendChild(p);
   }
}showMetrics();

//This is for testing things
function writeMsg(msg){
   var test = document.createElement('test');
      test.innerHTML = msg;
      document.body.appendChild(p);
}
</script>

</body>
</html>
 
