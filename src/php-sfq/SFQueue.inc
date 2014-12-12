<?php

class SFQueueClientException extends Exception
{
	static $mesgmap =
	[
		0	=> 'it has not yet been set',
		1010	=> 'extension(wrap_libsfq) is not loaded', 
		1020	=> 'it has not yet been initialized',
		1030	=> 'it does not yet implemented', 
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

interface SFQueueClientInterface
{
	public function push_text($params);
	public function push_binary($params);
	public function pop();
	public function shift();
}

abstract class SFQueueClient implements SFQueueClientInterface
{
	function __construct($quename, $throwException)
	{
		$this->quename_ = $quename;
		$this->throwException_ = $throwException;

		$this->initialized_ = true;
		$this->notInitialized_ = false;
	}

// protected:
	protected $initialized_ = false;
	protected $notInitialized_ = true;

	protected $quename_;
	protected $throwException_;

	protected $lastError_ = 0;
	protected $lastMessage_ = '';

	protected function throwException($code, $mesg=null)
	{
		$this->lastError_ = $code;
		$this->lastMessage_ = $mesg;

		if ($this->throwException_)
		{
			throw new SFQueueClientException($code, $mesg);
		}
	}

// public:
	public function lastError()
	{
		return $this->lastError_;
	}

	public function lastMessage()
	{
		return $this->lastMessage_;
	}
}

class SFQueueClientLocal extends SFQueueClient
{
	private $querootdir_;

	function __construct($quename, $params, $throwException)
	{
		if (extension_loaded('wrap_libsfq'))
		{
			if (isset($params['querootdir']))
			{
				$querootdir_ = $params['querootdir'];
			}

			if (isset($quename))
			{
				$quename_ = $quename;
			}

			parent::__construct($quename, $throwException);
		}
		else
		{
			$this->throwException(1010);
		}
	}

// private:
	private function isNativeSuccess_($rc)
	{
		$ret = false;

		if ($this->notInitialized_)
		{
			$this->throwException(1020);
		}
		else
		{
			if ($rc == SFQ_RC_SUCCESS)
			{
				$ret = true;
			}
			else if ($rc > SFQ_RC_FATAL_MIN)
			{
				$this->throwException(2000 + $rc, "native error rc={$rc}");
			}
		}

		return $ret;
	}

	private function native_push_($params)
	{
		$rc = wrap_sfq_push($this->querootdir_, $this->quename_, $params);

		if ($this->isNativeSuccess_($rc))
		{
			return $params['uuid'];
		}

		return false;
	}

	private function native_takeout_($funcname)
	{
		$param = [];

		$rc = wrap_sfq_takeout($this->querootdir_, $this->quename_, $param, $funcname);

		if ($this->isNativeSuccess_($rc))
		{
			return $param;
		}

		return false;
	}

// public:
	function push_text($params)
	{
		if (isset($params['payload']))
		{
			if (! isset($params['payload_type']))
			{
				$params['payload_type'] = (SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM);
			}
		}

		return $this->native_push_($params);
	}

	function push_binary($params)
	{
		if (isset($params['payload']))
		{
			if (! isset($params['payload_type']))
			{
				$params['payload_type'] = (SFQ_PLT_BINARY);
			}
		}

		return $this->native_push_($params);
	}

	function pop()
	{
		return $this->native_takeout_('sfq_pop');
	}

	function shift()
	{
		return $this->native_takeout_('sfq_shift');
	}
}

class SFQueue
{
	static function newClient($quename=null, $params=[])
	{
		if (isset($params['host']))
		{
			// connect remote host

			throw new SFQueueClientException(1030);
		}
		else
		{
			// connect localhost

			return new SFQueueClientLocal($quename, $params, true);
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
	// local (throw exception)
		try
		{
			$sfqc = SFQueue::newClient();

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

			var_dump($sfqc->push_text(['payload'=>'ccc 3 ddd']));
			echo "# rc= " . $sfqc->lastError() . PHP_EOL . PHP_EOL;
		}
		catch (Exception $ex)
		{
			echo 'CODE:' . $ex->getCode() . PHP_EOL;
			echo 'MESG:' . $ex->getMessage() . PHP_EOL;
		}

	// remote
/*
		$copt = [
			'host' => 'localhost',
			'port' => [
				'push'		=> 12701,
				'takeout'	=> 12702,
			],
		];

		$sfqc = SFQueue::newClient($copt);
*/
	}
}

// EOF

