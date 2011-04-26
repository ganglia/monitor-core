<?php
require_once 'eval_conf.php';
require_once 'auth.php';

if($conf['auth_system'] && isSet($_SERVER['REMOTE_USER']) && !empty($_SERVER['REMOTE_USER']) ){
  $auth = GangliaAuth::getInstance();
  $auth->setAuthCookie($_SERVER['REMOTE_USER']);
  header("Location: index.php");
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
    <?php if(!$conf['auth_system']) { ?>
      Authentication is disabled by Ganglia configuration.<br/>
      <code>$conf['auth_system'] = false;</code>
    <?php } else { ?>
      Your administrator may have disabled authentication, or it may not be configured correctly.<br/>
      This will occur if no authentication mechanism is configured in Apache.
    <?php } ?>
  </div>
</body>
</html>
