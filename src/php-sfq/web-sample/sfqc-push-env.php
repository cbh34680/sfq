<?php
	require 'common.inc';
	printHeaders();
?>
<html>
<body>
<?php
try
{
	$reqv = [
		'execpath' =>'env',
		'soutpath' => '-',
		'serrpath' => '-',
	];

	$sfqc = SFQueue::newClient('webque-1');
	$resp = $sfqc->push_binary($reqv);

	if ($resp)
	{
		if (preg_match_all('/../', implode(explode('-', $resp)), $matches))
		{
			$log = '/var/tmp/webque-1/logs/exec/' . implode('/', $matches[0]) . '/std.out';
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
<? if (isset($log)) { echo "<hr /><i><a href='cat.php?uuid={$resp}&ext=out' target='_blank'>see</a></i>: {$log}" . PHP_EOL; } ?>
</body>
</html>
