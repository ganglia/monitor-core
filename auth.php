<?php

/*
Levels of access control:
disable all auth, via config.  (behind firewall, everybody can do everything)
enable auth, subject to config rules
'edit' needs to check for writeability of data directory.  error log if edit is allowed but we're unable to due to fs problems.
*/
require_once 'eval_conf.php';
require_once 'lib/GangliaAcl.php';
require_once 'lib/GangliaAuth.php';

$acl = GangliaAcl::getInstance();

// define roles for all users
// specify the user, then the groups they belong to (if any)
// you can use GangliaAcl::ADMIN to grant all privileges, or make up your own groups using $acl->addRole('group-name')

// 'admin-user' is the name provided by Apache (from your htaccess file or other authentication system)
// $acl->addRole('admin-user',GangliaAcl::ADMIN);

// if you want per-cluster restrictions, define them like this
// $acl->addResource('cluster1',GangliaAcl::ALL);
// $acl->allow(GangliaAcl::ADMIN,'cluster1','edit');

//$acl->addPrivateCluster('private-cluster');

/**
 * Check if current user has a privilege (view, edit, etc) on a resource.
 * If resource is unspecified, we assume GangliaAcl::ALL.
 *
 * Examples
 *   checkAccess( 'edit', $conf ); // user has global edit?
 *   checkAccess( 'view', $conf ); // user has global view?
 *   checkAccess( 'edit', $cluster ); // user can edit current cluster?
 *   checkAccess( 'edit', 'cluster1', $conf ); // user has edit privilege on cluster1?
 *   checkAccess( 'view', 'cluster1', $conf ); // user has view privilege on cluster1?
 */
function checkAccess() {
  $args = func_get_args();
  $privilege = $args[0];
  switch(count($args)) {
    case 2:
      $resource=GangliaAcl::ALL;
      $conf = $args[1];
      break;
    case 3:
      $resource = $args[1];
      $conf = $args[2];
      break;
    default:
      trigger_error('checkAccess requires 2 or 3 arguments.',E_USER_ERROR);
      break;
  }
  
  // if auth system is disabled, everything is allowed.
  if(!$conf['auth_system']) {
    return true;
  }
  
  $acl = GangliaAcl::getInstance();
  $auth = GangliaAuth::getInstance();
  
  if(!$auth->isAuthenticated()) {
    $user = GangliaAcl::GUEST;
  } else {
    $user = $auth->getUser();
  }
  
  if(!$acl->has($resource)) {
    throw new Exception("Unknown resource '$resource'.");
  }
  if($acl->hasRole($user)) {
    return (bool) $acl->isAllowed($user, $resource, $privilege);
  }
  return false;
}

// echo "Edit:".checkAccess( 'edit', $conf );
// echo "View:".checkAccess( 'view', $conf );
// echo "View private:".checkAccess( 'view', 'private-cluster', $conf );
// echo "Edit private:".checkAccess( 'edit', 'private-cluster', $conf );

#
# Functions to authenticate users with the HTTP "Basic" password
# box. Original author Federico Sacerdoti <fds@sdsc.edu>
#

#-------------------------------------------------------------------------------
# Returns an array of clusters that want to be private.
# Get list of private clusters. Put in $private[cluster name]="password"
function embarrassed ()
{
   # The @ in front of a function name suppresses any warnings from it.
   $fp=@fopen("./private_clusters","r");
   if ($fp) {
      while(!feof($fp)) {
         $line=chop(fgets($fp,255));
         if (!$line or !strcspn($line,"#")) { continue; }
         $list=explode("=",$line);
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
