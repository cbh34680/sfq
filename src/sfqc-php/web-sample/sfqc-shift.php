<?php
	require 'common.inc';
	printHeaders();
?>
<html>
<body>
<?php include_once("analyticstracking.php") ?>
<?php
try
{
	$sfqc = SFQueue::newClient([ 'quename' => 'webque-0' ]);
	$resp = $sfqc->shift();
}
catch (Exception $ex)
{
}

?>
<? if (isset($ex)) : ?>
<hr />
CODE: <?= $ex->getCode() ?><br />
MESG: <?= $ex->getMessage() ?><br />
<? endif ?>
<hr />
<pre>
<? if (isset($reqv)) { var_dump($reqv); } ?>
<?= var_dump(@$resp) ?>
</pre>
</body>
</html>
