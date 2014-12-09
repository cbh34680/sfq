<?php

$ioparam = [ ];

foreach (['sfq_pop', 'sfq_shift'] as $func)
{
	$rc = wrap_sfq_takeout(null, null, $ioparam, $func);

	if ($rc == SFQ_RC_SUCCESS)
	{
		$id = $ioparam['id'];
		$uuid = $ioparam['uuid'];
		$execpath = $ioparam['execpath'];
		$execargs = $ioparam['execargs'];
		$metatext = $ioparam['metatext'];
		$soutpath = $ioparam['soutpath'];
		$serrpath = $ioparam['serrpath'];
		$payload_size = $ioparam['payload_size'];
		$payload_type = $ioparam['payload_type'];
		$payload = $ioparam['payload'];

		if ($payload)
		{
			file_put_contents("output-{$func}.bin", $payload);
		}

		echo "({$func})success" . PHP_EOL;

		echo <<< END_DOC
	id		:{$id}
	uuid		:{$uuid}
	execpath	:{$execpath}
	execargs	:{$execargs}
	metatext	:{$metatext}
	soutpath	:{$soutpath}
	serrpath	:{$serrpath}
	payload_size	:{$payload_size}
	payload_type	:{$payload_type}

END_DOC;
	}
	else
	{
		echo "({$func})error rc=[{$rc}]" . PHP_EOL;
	}
}

exit (0);

