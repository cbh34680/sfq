<?php

declare(ticks = 1);

require 'SFQueue.inc';

$childs = [];

for ($i=0; $i<5; $i++)
{
	$pid = pcntl_fork();

	if ($pid == -1)
	{
		continue;
	}

	if ($pid)
	{
		$childs[] = $pid;
	}
	else
	{
// child
		$cont = true;

		$hndl = function ($signo) use (&$cont)
		{
			$cont = false;
		};

		pcntl_signal(SIGHUP, $hndl);
		pcntl_signal(SIGINT, $hndl);
		pcntl_signal(SIGTERM, $hndl);

		try
		{
			$sfqc = SFQueue::newClient();

			echo "c-{$i}) while start" . PHP_EOL;
			while ($cont)
			{
#echo "PSH({$i}) ";
				$sfqc->push_binary(['payload'=>"{$i}"]);

#echo "PSH({$i}) ";
				$sfqc->push_binary(['payload'=>"{$i}"]);

				$v = $sfqc->shift();
				if ($v)
				{
					$payload = $v['payload'];
					$eq = ($i == $payload) ? '=' : '*';

#echo "SHF({$i})({$payload}){$eq} ";
				}
			}

			echo "c-{$i}) while end" . PHP_EOL;
		}
		catch (Exception $e)
		{
			echo "Exception({$i})" . PHP_EOL;
			echo $e->getTraceAsString();

			exit(1);
		}

		exit(0);
	}
}

/*
echo "*" . PHP_EOL;
echo "* stop when you press the [Enter]." . PHP_EOL;
echo "*" . PHP_EOL;
echo PHP_EOL;

try
{
	readline("");
}
catch (Exception $ex)
{
}

#
echo PHP_EOL;
echo "! send stop signal to child process" . PHP_EOL;

foreach ($childs as $key=>$pid)
{
	$sendsig = SIGINT;

	echo "\tsend kill to pid={$pid} send={$sendsig}" . PHP_EOL;
	posix_kill($pid, $sendsig);
}
echo PHP_EOL;
*/

#
echo "! wait for exit of child processs" . PHP_EOL;

while (count($childs))
{
	foreach ($childs as $key=>$pid)
	{
		$res = pcntl_waitpid($pid, $status, WNOHANG);

		if (($res == -1) || ($res > 0))
		{
			unset($childs[$key]);

			//
			echo "\tfinish child pid={$pid} status={$status}" . PHP_EOL;

			if (pcntl_wifexited($status))
			{
				echo "\t\tcause=exit exit-status=" . pcntl_wexitstatus($status) . PHP_EOL;
			}
			else if (pcntl_wifstopped($status))
			{
				echo "\t\tcause=stop stop-signo=" . pcntl_wstopsig($status) . PHP_EOL;
			}
			else if (pcntl_wifsignaled($status))
			{
				echo "\t\tcause=signal term-signo=" . pcntl_wtermsig($status) . PHP_EOL;
			}
		}
	}

	sleep(1);
}
echo PHP_EOL;

echo "ALL DONE." . PHP_EOL . PHP_EOL;

exit (0);

