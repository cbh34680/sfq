<?php

$file = $argv[1];
$data = file_get_contents($file);
$dlen = strlen($data);

echo "input data file={$file} size={$dlen}" . PHP_EOL;

$ioparam =
[
	'metatext'	=> $file,
	'payload'	=> $data,
	//'payload_size'	=> $dlen,
	'payload_type'	=> SFQ_PLT_BINARY,
];

for ($i=0; $i<2; $i++)
{
	$rc = wrap_sfq_push(null, null, $ioparam);

	if ($rc == SFQ_RC_SUCCESS)
	{
		$uuid = $ioparam['uuid'];

		echo "({$i})success, uuid=[{$uuid}]" . PHP_EOL;
	}
	else
	{
		echo "({$i})error, rc=[{$rc}]" . PHP_EOL;
		exit (1);
	}
}

exit (0);

