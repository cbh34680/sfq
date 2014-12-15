<?php

// 
interface SFQueueClientInterface
{
	public function push_text($params);
	public function push_binary($params);
	public function pop();
	public function shift();
}

//
class SFQueueClientException extends Exception
{
	static $mesgmap =
	[
		0	=> 'it has not yet been set',
		1010	=> 'extension(wrap_libsfq) is not loaded', 
		1020	=> 'it has not yet been initialized',
		1030	=> 'it does not yet implemented', 
		1040	=> 'illegal argument type exception', 
	];

	function __construct($code, $mesg=null)
	{
		if (is_null($mesg))
		{
			$mesg = self::c2m($code);
		}

		parent::__construct($mesg, $code);
	}

	static function c2m($code)
	{
		if (isset(self::$mesgmap[$code]))
		{
			return self::$mesgmap[$code];
		}

		return "unknown code ({$code})";
	}
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
		$this->lastMessage_ = $mesg ? $mesg : SFQueueClientException::c2m($code);

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
		if (is_array($params))
		{
			$rc = wrap_sfq_push($this->querootdir_, $this->quename_, $params);

			if ($this->isNativeSuccess_($rc))
			{
				return $params['uuid'];
			}
		}
		else
		{
			$this->throwException(1040);
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
				$params['payload_type'] = (SFQ_PLT_CHARARRAY);
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

class SFQueueClientRemote extends SFQueueClient
{
	function __construct($quename, $params, $throwException)
	{
		parent::__construct($quename, $throwException);
	}

	function push_text($params)
	{
		$this->throwException(1030);
	}

	function push_binary($params)
	{
		$this->throwException(1030);
	}

	function pop()
	{
		$this->throwException(1030);
	}

	function shift()
	{
		$this->throwException(1030);
	}
}

class SFQueue
{
	static function newClient($quename=null, $params=[])
	{
		if (isset($params['host']))
		{
			// connect remote host

			return new SFQueueClientRemote($quename, $params, true);
		}
		else
		{
			// connect localhost

			return new SFQueueClientLocal($quename, $params, true);
		}
	}

	static function newClientNotThrow($quename=null, $params=[])
	{
		if (isset($params['host']))
		{
			// connect remote host

			return new SFQueueClientRemote($quename, $params, false);
		}
		else
		{
			// connect localhost

			return new SFQueueClientLocal($quename, $params, false);
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
		$test = function($sfqc)
		{
			$sres = function() use ($sfqc)
			{
				echo "! e= " . $sfqc->lastError() . PHP_EOL;
				echo "! m= " . $sfqc->lastMessage() . PHP_EOL;
				echo PHP_EOL;
			};

			//
			$cn = get_class($sfqc);
			echo '/ ' . str_repeat('-', 80) . PHP_EOL;

			echo <<< EOS
/
/ Class Name = [{$cn}]
/

EOS;
			echo '/ ' . str_repeat('-', 80) . PHP_EOL;

			var_dump($sfqc->pop());
			$sres();

			var_dump($sfqc->push_text(['payload'=>'aaa 1 bbb', 'execpath'=>'']));
			$sres();

			var_dump($sfqc->push_text(['payload'=>'ccc 2 ddd']));
			$sres();

			var_dump($sfqc->pop());
			$sres();

			var_dump($sfqc->shift());
			$sres();

			var_dump($sfqc->push_text(['payload'=>'ccc 3 ddd']));
			$sres();

			var_dump($sfqc->push_text(1));
			$sres();

			echo "! ALL DONE." . PHP_EOL;
		};

		$etest = function($sfqc) use ($test)
		{
			try
			{
				$test($sfqc);
			}
			catch (Exception $ex)
			{
				$AST = str_repeat('*', 25);

				echo "{$AST} EXCEPTION {$AST}" . PHP_EOL;

				echo 'CODE: ' . $ex->getCode() . PHP_EOL;
				echo 'MESG: ' . $ex->getMessage() . PHP_EOL;
				echo 'FILE: ' . $ex->getFile() . PHP_EOL;
				echo 'LINE: ' . $ex->getLine() . PHP_EOL;
				echo PHP_EOL;
				echo '[TRACE]' . PHP_EOL;
				echo $ex->getTraceAsString();
				echo PHP_EOL;
			}
		};

	// local (throw exception)

		$etest(SFQueue::newClient());
		echo PHP_EOL;

	// local (not throw exception)

		$test(SFQueue::newClientNotThrow());
		echo PHP_EOL;

	// remote (throw exception)
		$copt = [
			'host' => 'localhost',
			'port' => [
				'push'		=> 12701,
				'takeout'	=> 12702,
			],
		];

		$etest(SFQueue::newClient(null, $copt));
		echo PHP_EOL;

	// remote (not throw exception)

		$test(SFQueue::newClientNotThrow(null, $copt));
		echo PHP_EOL;
	}
}

// EOF

