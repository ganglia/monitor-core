<?php
require_once 'Zend/Acl.php';

class GangliaAcl extends Zend_Acl {
  private static $acl;
  
  const ALL   = 'all';
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
  
  private function __construct() {
    // define default groups
    $this->addRole( new Zend_Acl_Role(GangliaAcl::GUEST))
         ->addRole( new Zend_Acl_Role(GangliaAcl::ADMIN));
    
    // define default resources
    // all clusters should be children of GangliaAcl::ALL
    $this->add( new Zend_Acl_Resource(GangliaAcl::ALL) );
    
    // guest can view everything.  (private clusters are set up using $this->deny())
    $this->allow(GangliaAcl::GUEST, GangliaAcl::ALL, GangliaAcl::VIEW);
    $this->allow(GangliaAcl::ADMIN, GangliaAcl::ALL, GangliaAcl::VIEW);
    // admin can edit everything.
    $this->allow(GangliaAcl::ADMIN, GangliaAcl::ALL, GangliaAcl::EDIT);
  }
  
  public function addPrivateCluster($cluster) {
    
    $this->add( new Zend_Acl_Resource($cluster), self::ALL );
    //$this->allow(self::ADMIN, $cluster, 'edit');
    $this->deny(self::GUEST, $cluster);
  }
}
?>