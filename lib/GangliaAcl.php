<?php
require_once 'Zend/Acl.php';

class GangliaAcl extends Zend_Acl {
  private static $acl;
  
  const ALL_CLUSTERS   = 'all';
  const VIEW  = 'view';
  const EDIT  = 'edit';
  const ADMIN = 'admin';
  const GUEST = 'guest';
  
  public static function getInstance() {
    if(is_null(self::$acl)) {
      self::$acl = new GangliaAcl();
    }
    return self::$acl;
  }
  
  public function __construct() {
    // define default groups
    $this->addRole( new Zend_Acl_Role(GangliaAcl::GUEST))
         ->addRole( new Zend_Acl_Role(GangliaAcl::ADMIN));
    
    // define default resources
    // all clusters should be children of GangliaAcl::ALL_CLUSTERS
    $this->add( new Zend_Acl_Resource(GangliaAcl::ALL_CLUSTERS) );
    
    // guest can view everything.  (private clusters are set up using $this->deny())
    $this->allow(GangliaAcl::GUEST, GangliaAcl::ALL_CLUSTERS, GangliaAcl::VIEW);
    $this->allow(GangliaAcl::ADMIN, GangliaAcl::ALL_CLUSTERS, GangliaAcl::VIEW);
    // admin can edit everything.
    $this->allow(GangliaAcl::ADMIN, GangliaAcl::ALL_CLUSTERS, GangliaAcl::EDIT);
  }
  
  public function addPrivateCluster($cluster) {
    
    $this->add( new Zend_Acl_Resource($cluster), self::ALL_CLUSTERS );
    //$this->allow(self::ADMIN, $cluster, 'edit');
    $this->deny(self::GUEST, $cluster);
  }
}
?>