<?php
require_once 'Zend/Acl.php';

class GangliaAcl extends Zend_Acl {
  private static $acl;
  
  // resources
  const ALL_RESOURCES = 'all_resources';
  const  ALL_CLUSTERS = 'all_clusters';
  const     ALL_VIEWS = 'all_views';
  
  // privileges
  const          VIEW = 'view';
  const          EDIT = 'edit';
  
  // roles
  const         ADMIN = 'admin';
  const         GUEST = 'guest';
  
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
    $this->add( new Zend_Acl_Resource(GangliaAcl::ALL_RESOURCES) );
    $this->add( new Zend_Acl_Resource(GangliaAcl::ALL_CLUSTERS), GangliaAcl::ALL_RESOURCES);
    $this->add( new Zend_Acl_Resource(GangliaAcl::ALL_VIEWS), GangliaAcl::ALL_RESOURCES);
    
    // guest can view everything and edit nothing.
    $this->allow(GangliaAcl::GUEST, GangliaAcl::ALL_RESOURCES, GangliaAcl::VIEW);
    $this->deny(GangliaAcl::GUEST, GangliaAcl::ALL_RESOURCES, GangliaAcl::EDIT);
    
    $this->allow(GangliaAcl::ADMIN, GangliaAcl::ALL_RESOURCES, GangliaAcl::EDIT);
    $this->allow(GangliaAcl::ADMIN, GangliaAcl::ALL_RESOURCES, GangliaAcl::VIEW);
  }
  
  public function addPrivateCluster($cluster) {
    $this->add( new Zend_Acl_Resource($cluster), self::ALL_CLUSTERS );
    //$this->allow(self::ADMIN, $cluster, 'edit');
    $this->deny(self::GUEST, $cluster);
  }
}
?>