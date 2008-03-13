<?php
#
# Functions to authenticate users with the HTTP "Basic" password
# box. Original author Federico Sacerdoti <fds@sdsc.edu>
#

#-------------------------------------------------------------------------------
# Returns an array of clusters that want to be private.
# Get list of private clusters. Put in $private[cluster name]="password"
function embarrassed ()
{
   global $gmetad_root;
   # The @ in front of a function name suppresses any warnings from it.
   $fp=@fopen("./private_clusters","r");
   if ($fp) {
      while(!feof($fp)) {
         $line=chop(fgets($fp,255));
         if (!$line or !strcspn($line,"#")) { continue; }
         $list=split("=",$line);
         if (count($list)!=2) { continue; }
         $name=trim($list[0]);
         $pass=trim($list[1]);
         $private[$name] = $pass;
      }
      fclose($fp);
   }
   return $private;
}

#-------------------------------------------------------------------------------
function authenticate()
{
   global $clustername, $cluster;

   $private_clusters = array_keys( embarrassed() );

   if( in_array( $clustername, $private_clusters ) && ( $clustername == $cluster['NAME'] ) )
   {
      $auth_header	= "WWW-authenticate: basic realm=\"Ganglia Private Cluster: " . $clustername . "\"";
   }
   else
   {
      $auth_header	= "WWW-authenticate: basic realm=\"Ganglia Private Cluster\"";
   }

   header( $auth_header );
   header("HTTP/1.0 401 Unauthorized");
   #print "<HTML><HEAD><META HTTP-EQUIV=refresh CONTENT=1 URL=\"../?c=\"></HEAD>";
   print "<H1>You are unauthorized to view the details of this Cluster</H1>";
   print "In order to access this cluster page you will need a valid name and ".
      "password.<BR>";
   print "<H4><A HREF=\"./\">Back to Meta Cluster page</A></H4>";
   exit;
}

#-------------------------------------------------------------------------------
function checkprivate()
{
   global $clustername, $context;

   # Allow the Meta context page.
   if ($context=="meta") { return; }

   $private=embarrassed();
   if (isset($private[$clustername]) and $private[$clustername]) {
      #echo "The password for $clustername is $private[$clustername]<br>";
      if (empty($_SERVER['PHP_AUTH_PW'])) {
	 authenticate();
      }
      else {
	 # Check password (in md5 format). Username does not matter.
	 if (md5($_SERVER['PHP_AUTH_PW']) != $private[$clustername]) {
	    authenticate();
	 }
      }
   }
}
 
#-------------------------------------------------------------------------------
# To be called when in the control context. Assumes the password file
# "$gmetad_root/etc/private_clusters has an entry called "controlroom".
# The control room is always embarrassed.
function checkcontrol()
{
   global $context;

   if ($context != "control") { return; }

   if (empty($_SERVER['PHP_AUTH_PW'])) {
      authenticate();
   }
   else {
      #echo "You entered password ". md5($PHP_AUTH_PW) ." ($PHP_AUTH_PW)<br>";
      $private=embarrassed();
      if (md5($_SERVER['PHP_AUTH_PW']) != $private["controlroom"]) {
	 authenticate();
      }
   }
}

?>
