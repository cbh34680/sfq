<?php
	require 'common.inc';
	printHeaders();
?>
<html>
<body>
<?php include_once("analyticstracking.php") ?>
<a href="https://github.com/cbh34680/sfq/blob/master/src/sfqc-php/web-sample/<?= basename(__FILE__) ?>" target="_blank">source</a><br />
<?php

try
{
	$reqv = [
		'payload' => date('Y-m-d H:i:s'),
		'metatext' => @$_SERVER['REMOTE_ADDR'],
	];

	$sfqc = SFQueue::newClient([ 'quename' => 'webque-0' ]);
	$resp = $sfqc->push_text($reqv);
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
