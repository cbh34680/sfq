<?php
	require 'common.inc';
	printHeaders();

	$uuid = @$_GET['uuid'];
	$file = @$_GET['file'];

	if ($uuid && $file)
	{
		if (preg_match('/^[0-9A-Za-z-]+$/', $uuid) && preg_match('/^[0-9A-Za-z-]+\.[a-z]{3}/', $file))
		{
			if (preg_match_all('/../', implode(explode('-', $uuid)), $matches))
			{
				$path = '/var/tmp/webque-1/logs/exec/' . implode('/', $matches[0]) . '/' . $file;

				if (! file_exists($path))
				{
					unset($path);
				}
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
<i>file:</i> <?= $file ?><br />
<i>path:</i> <? if (isset($path)) { echo "{$path} (ts=" . date('y/m/d H:i:s', filemtime($path)) . ")"; } ?><br />
<br />
<hr />
<blockquote>
<pre>
<? if (isset($path)) { readfile($path); } ?>
</pre>
</blockquote>
<hr />
</body>
</html>
