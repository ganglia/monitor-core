<?php
require_once 'eval_conf.php';

if($conf['auth_system'] == 'enabled' && isSet($_SERVER['REMOTE_USER']) && !empty($_SERVER['REMOTE_USER']) ){
  $auth = GangliaAuth::getInstance();
  $auth->setAuthCookie($_SERVER['REMOTE_USER']);
  $redirect_to = isSet( $_SERVER['HTTP_REFERER'] ) ? $_SERVER['HTTP_REFERER'] : 'index.php';
  header("Location: $redirect_to");
  die();
}
?>
<html>
<head>
  <title>Authentication Failed</title>
</head>
<body>
  <h1>We were unable to log you in.</h1>
  <div>
    <?php if( ! isSet($conf['auth_system'] ) ) { ?>
      <code>$conf['auth_system']</code> is not defined.<br/>  Please notify an administrator.
    <?php } else if($conf['auth_system'] == 'disabled' || $conf['auth_system'] == 'readonly') { ?>
      Authentication is disabled by Ganglia configuration.<br/>
      <code>$conf['auth_system'] = '<?php echo $conf['auth_system']; ?>';</code>
    <?php } else { ?>
      Authentication is not configured correctly.  The web server must provide an authenticated username.
    <?php } ?>
  </div>
</body>
</html>
