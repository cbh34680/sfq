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
	$sfqc = SFQueue::newClient([ 'quename' => 'webque-0' ]);
	$resp = $sfqc->pop();
}
catch (Exception $ex)
{
}

?>
<?php if (isset($ex)) : ?>
<hr />
CODE: <?= $ex->getCode() ?><br />
MESG: <?= $ex->getMessage() ?><br />
<?php endif ?>
<hr />
<pre>
<?php if (isset($reqv)) { var_dump($reqv); } ?>
<?= var_dump(@$resp) ?>
</pre>
</body>
</html>
