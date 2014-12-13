<?php
	require 'common.inc';
	printHeaders();
?>
<html>
<body>
<a href="https://github.com/cbh34680/sfq/blob/master/src/php-sfq/web-sample/<?= basename(__FILE__) ?>" target="_blank">source</a><br />
<?php
try
{
	$reqv = [
		'payload'     =>'date;date;date',
		'payload_size' => 10,
		'soutpath'     => '-',
		'serrpath'     => '-',
	];

	$sfqc = SFQueue::newClient('webque-1');
	$uuid = $sfqc->push_binary($reqv);

	if ($uuid)
	{
		if (preg_match_all('/../', implode(explode('-', $uuid)), $matches))
		{
			$dir = '/var/tmp/webque-1/logs/exec/' . implode('/', $matches[0]);
		}
	}
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
<? if (isset($dir)) : ?>
<hr />
<i><a href='cat.php?uuid=<?= $uuid ?>&file=id.txt'  target='_blank'>see</a></i>: <?= $dir?>/id.txt<br />
<i><a href='cat.php?uuid=<?= $uuid ?>&file=std.out' target='_blank'>see</a></i>: <?= $dir?>/std.out<br />
<i><a href='cat.php?uuid=<?= $uuid ?>&file=std.err' target='_blank'>see</a></i>: <?= $dir?>/std.err<br />
<i><a href='cat.php?uuid=<?= $uuid ?>&file=rc.txt'  target='_blank'>see</a></i>: <?= $dir?>/rc.txt<br />
<? endif ?>
</body>
</html>
