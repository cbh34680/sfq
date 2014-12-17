--TEST--
wrap_sfq_push() function
--SKIPIF--
<?php 

if(!extension_loaded('sfqc_php')) die('skip ');

 ?>
--FILE--
<?php
$ioparam = [ 'execpath' => 'cc', 'execargs' => 'dd' ];
echo wrap_sfq_push(null, null, $ioparam);

?>
--EXPECT--
0
