<?php

function printHeaders()
{
	header('Content-type: text/html; charset=UTF-8');
	header('Last-Modified: ' . gmdate('D, d M Y H:i:s') . ' GMT');
	header('Expires: Thu, 19 Nov 1981 08:52:00 GMT');
	header('Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0');
	header('Pragma: no-cache');
}

function myAutoloader($name)
{
	if (! $name)
	{
		return;
	}

	$ret = @include_once($name . '.inc');
	if (! $ret)
	{
		$ret = @include_once($name . '.php');
		if (! $ret)
		{
			$ret = @include_once($name . '.class.php');
		}
	}
}

spl_autoload_register('myAutoloader');



