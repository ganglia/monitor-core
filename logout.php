<?php
require 'auth.php';
$auth = GangliaAuth::getInstance();
$auth->destroyAuthCookie();
header("Location: index.php");
?>