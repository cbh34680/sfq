<?php
	require 'common.inc';
	printHeaders();

	$uuid = @$_GET['uuid'];
	$ext = @$_GET['ext'];

	if ($uuid && $ext)
	{
		if (preg_match('/^[0-9A-Za-z-]+$/', $uuid) && in_array($ext, ['out', 'err']))
		{
			if (preg_match_all('/../', implode(explode('-', $uuid)), $matches))
			{
				$log = '/var/tmp/webque-1/logs/exec/' . implode('/', $matches[0]) . '/std.' . $ext;
			}
		}
	}
?>
<html>
<head>
<title><?= $uuid ?></title>
</head>
<body>
<i>uuid:</i> <?= $uuid ?><br />
<i>ext:</i> <?= $ext ?><br />
<i>file:</i> <? if (isset($log)) { echo "{$log} (ts=" . date('y/m/d H:i:s', filemtime($log)) . ")"; } ?><br />
<br />
<hr />
<blockquote>
<pre>
<? if (isset($log)) { readfile($log); } ?>
</pre>
</blockquote>
<hr />
</body>
</html>
