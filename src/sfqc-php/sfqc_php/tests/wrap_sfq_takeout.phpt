--TEST--
wrap_sfq_takeout() function
--SKIPIF--
<?php 

if(!extension_loaded('sfqc_php')) die('skip ');

 ?>
--FILE--
<?php
$ioparam = [ ];
echo wrap_sfq_takeout(null, null, $ioparam, 'sfq_pop');

?>
--EXPECT--
0
