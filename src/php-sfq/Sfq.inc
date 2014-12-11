<?php

class SfqClientException extends Exception
{
	static $mesgmap =
	[
		1010	=> 'extension(wrap_libsfq) is not loaded', 
		1020	=> 'call unknown method', 
		1030	=> 'It does not yet implemented', 
	];

	function __construct($code, $mesg=null)
	{
		if (! isset($mesg))
		{
			if (isset(self::$mesgmap[$code]))
			{
				$mesg = self::$mesgmap[$code];
			}
		}

		if (! isset($mesg))
		{
			$mesg = "unknown code ({$code})";
		}

		parent::__construct($mesg, $code);
	}
}

interface SfqClientInterface
{
	public function push_text($params);
	public function push_binary($params);
	public function pop();
	public function shift();
}

abstract class SfqClient implements SfqClientInterface
{
	function __construct()
	{
	}

	protected $lastError_;

	public function lastError()
	{
		return $this->lastError_;
	}
}

class SfqClientLocal extends SfqClient
{
	private $quename;
	private $querootdir;

	function __construct($quename, $params)
	{
		if (! extension_loaded('wrap_libsfq'))
		{
			throw new SfqClientException(1010);
		}

		if (isset($params['querootdir']))
		{
			$this->querootdir = $params['querootdir'];
		}

		if (isset($quename))
		{
			$this->quename = $quename;
		}

		parent::__construct();
	}

	private function isSuccess($rc)
	{
		if ($rc > SFQ_RC_FATAL_MIN)
		{
			throw new SfqClientException(2000 + $rc, "native error rc={$rc}");
		}

		$this->lastError_ = $rc;

		return ($rc == SFQ_RC_SUCCESS);
	}

	private function push($params)
	{
		$rc = wrap_sfq_push($this->querootdir, $this->quename, $params);

		if ($this->isSuccess($rc))
		{
			return $params['uuid'];
		}

		return false;
	}

	function push_text($params)
	{
		$params['payload_type'] = (SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM);

		return $this->push($params);
	}

	function push_binary($params)
	{
		$params['payload_type'] = (SFQ_PLT_BINARY);

		return $this->push($params);
	}

	private function takeout($funcname)
	{
		$param = [];

		$rc = wrap_sfq_takeout($this->querootdir, $this->quename, $param, $funcname);

		if ($this->isSuccess($rc))
		{
			return $param;
		}

		return false;
	}

	function pop()
	{
		return $this->takeout('sfq_pop');
	}

	function shift()
	{
		return $this->takeout('sfq_shift');
	}
}

class Sfq
{
	static function newClient($quename=null, $params=[])
	{
		if (isset($params['host']))
		{
			// connect remote host

			throw new SfqClientException(1030);
		}
		else
		{
			// connect localhost

			return new SfqClientLocal($quename, $params);
		}
	}

}

//
// class test
//
if (isset($argv))
{
	if (realpath($argv[0]) == __FILE__)
	{
	// local
		$sfqc = Sfq::newClient();

		var_dump($sfqc->pop());
		echo "# rc= " . $sfqc->lastError() . PHP_EOL . PHP_EOL;

		var_dump($sfqc->push_text(['payload'=>'aaa 1 bbb', 'execpath'=>'']));
		echo "# rc= " . $sfqc->lastError() . PHP_EOL . PHP_EOL;

		var_dump($sfqc->push_text(['payload'=>'ccc 2 ddd']));
		echo "# rc= " . $sfqc->lastError() . PHP_EOL . PHP_EOL;

		var_dump($sfqc->pop());
		echo "# rc= " . $sfqc->lastError() . PHP_EOL . PHP_EOL;

		var_dump($sfqc->shift());
		echo "# rc= " . $sfqc->lastError() . PHP_EOL . PHP_EOL;

	// remote
		$copt = [
			'host' => 'localhost',
			'port' => [
				'push'		=> 12701,
				'takeout'	=> 12702,
			],
		];

#		$sfqc = Sfq::newClient($copt);
	}
}

// EOF

