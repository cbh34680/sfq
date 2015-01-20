<?php

//
interface SFQueueClientInterface
{
	public function push_text($params);
	public function push_binary($params);
	public function pop();
	public function shift();
}

// ---------------------------------------------------------------------------
//

class SFQueueClientException extends Exception
{
	static $mesgmap =
	[
		0		=> 'it has not yet been set',
		1010	=> 'extension(sfqc_php) is not loaded',
		1020	=> 'it has not yet been initialized',
		1030	=> 'it does not yet implemented',
		1040	=> 'illegal argument type exception',
		1050	=> 'unknown error',
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

// ---------------------------------------------------------------------------
//

abstract class SFQueueClient implements SFQueueClientInterface
{
	function __construct($throwException)
	{
		$this->throwException_ = $throwException;

		$this->notInitialized_ = false;
	}

// protected:
	protected $notInitialized_ = true;

	protected $throwException_;

	private $lastError_ = 0;
	private $lastMessage_ = '';

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

	public function clearLastError()
	{
		$this->lastError_ = 0;
		$this->lastMessage_ = '';
	}
}

// ---------------------------------------------------------------------------
//

class SFQueueClientLocal extends SFQueueClient
{
	protected $quename_;
	private $querootdir_;

	function __construct($params, $throwException)
	{
		if (extension_loaded('sfqc_php'))
		{
			parent::__construct($throwException);

			if (is_array($params))
			{
				$this->querootdir_ = @$params['querootdir'];
				$this->quename_ = @$params['quename'];
			}
		}
		else
		{
			$this->notInitialized_ = true;

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
		$this->clearLastError();

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
		$this->clearLastError();

		$params = [];
		$rc = wrap_sfq_takeout($this->querootdir_, $this->quename_, $params, $funcname);

		if ($this->isNativeSuccess_($rc))
		{
			return $params;
		}

		return false;
	}

// public:
	function push_text($params)
	{
		if (isset($params['payload']))
		{
			$params['payload_type'] = 'text';
		}

		return $this->native_push_($params);
	}

	function push_binary($params)
	{
		if (isset($params['payload']))
		{
			$params['payload_type'] = 'binary';
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

// ---------------------------------------------------------------------------
//

class SFQueueClientRemote extends SFQueueClient
{
	const CRLF = "\r\n";
	const LF = "\n";

	private $conn_params = [];

	function __construct($params, $throwException)
	{
		parent::__construct($throwException);

		if (is_array($params))
		{
			$this->conn_params = $params;
		}
	}

// private:
	private static function hostname2addr($arg)
	{
		$ret = false;

		if ($arg)
		{
			//
			// 必要なら IP アドレスに書き換え
			//
			if ($arg === 'localhost')
			{
				return '127.0.0.1';
			}

			if (! preg_match('/\A[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+\Z/', $arg))
			{
				$addr = gethostbyname($arg);
				if ($addr != $arg)
				{
					//
					// アドレスが解決できない
					//
					$ret = $addr;
				}
			}
		}

		return $ret;
	}

	private function isRemoteSuccess_($arg)
	{
		$ret = false;

		if ($this->notInitialized_)
		{
			$this->throwException(1020);
		}
		else
		{
			if (! isset($arg['result_code']))
			{
				$this->throwException(1050);
			}

			$rc = intval($arg['result_code']);

			if ($rc == 0 /* SFQ_RC_SUCCESS */)
			{
				$ret = true;
			}
			else if ($rc > 21 /* SFQ_RC_FATAL_MIN */)
			{
				$this->throwException(2000 + $rc, "native error rc={$rc}");
			}
		}

		return $ret;
	}

	private static function parse_response($arg)
	{
		//
		$header = [];
		$body = null;

		$sep_offset = strpos($arg, self::CRLF . self::CRLF);
		if ($sep_offset === false)
		{
			$sep_offset = strpos($arg, self::LF . self::LF);
		}

		if ($sep_offset === false)
		{
			$ret['body'] = $arg;
		}
		else
		{
			if ($sep_offset)
			{
				$header_text = substr($arg, 0, $sep_offset);

				foreach (explode(self::LF, $header_text) as $line)
				{
					if (! preg_match('/^\\s*([^:[:blank:]]+)\\s*:\\s*(\\S+)\\s*$/', trim($line), $matches))
					{
						continue;
					}

					list($all, $key, $val) = $matches;

					$key = str_replace('-', '_', $key);
					$header[$key] = $val;
				}
			}

			$body = substr($arg, $sep_offset + 4);
		}

		return [ 'header' => $header, 'body' => $body ];
	}

	function remote_io($opname, $params=[])
	{
		$host = $this->conn_params['host'];
		$port = $this->conn_params['port'][$opname];

		$to_read = $this->conn_params['timeout']['read'];
		$to_write = $this->conn_params['timeout']['write'];

		//
		$addr = self::hostname2addr($host);

		//
		if (isset($this->conn_params['querootdir']))
		{
			$params['querootdir'] = $this->conn_params['querootdir'];
		}

		if (isset($this->conn_params['quename']))
		{
			$params['quename'] = $this->conn_params['quename'];
		}

		$body = null;
		if (isset($params['payload']))
		{
			$body = $params['payload'];

			if (! isset($params['payload-length']))
			{
				$params['payload-length'] = strlen($body);
			}
		}

		//
		$header_arr = [];

		foreach ($params as $k=>$v)
		{
			if ($k === 'payload')
			{
				continue;
			}

			$header_arr[] = sprintf("%s: %s", str_replace('_', '-', $k), $v);
		}

		$header = implode(self::CRLF, $header_arr);

		$sep = self::CRLF . self::CRLF;

		//
		$sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);

		socket_connect($sock, $addr, $port);

		socket_set_option($sock, SOL_SOCKET, SO_RCVTIMEO, $to_read);
		socket_set_option($sock, SOL_SOCKET, SO_SNDTIMEO, $to_write);

		socket_write($sock, $header, strlen($header));
		socket_write($sock, $sep, strlen($sep));

		if (isset($body))
		{
			socket_write($sock, $body, strlen($body));
		}

		$resp_text = '';

		do
		{
			$buff = socket_read($sock, 8192);
			if ($buff !== false)
			{
				$resp_text .= $buff;
			}
		}
		while (! empty($buff));

		socket_shutdown($sock, 2);
		socket_close($sock);

		//
		return self::parse_response($resp_text);
	}

	private function remote_push($params)
	{
		$this->clearLastError();

		if (is_array($params))
		{
			$resp = $this->remote_io('push', $params);

			if ($this->isRemoteSuccess_($resp['header']))
			{
				return $resp['header']['uuid'];
			}
		}
		else
		{
			$this->throwException(1040);
		}

		return false;
	}

	function remote_takeout($opname)
	{
		$this->clearLastError();

		$resp = $this->remote_io($opname);

		if ($this->isRemoteSuccess_($resp['header']))
		{
			$ret = $resp['header'];
			$ret['payload'] = $resp['body'];

			return $ret;
		}

		return false;
	}

// public:
	function push_text($params)
	{
		if (isset($params['payload']))
		{
			$params['payload-type'] = 'text';
		}

		return $this->remote_push($params);
	}

	function push_binary($params)
	{
		if (isset($params['payload']))
		{
			$params['payload-type'] = 'binary';
		}

		return $this->remote_push($params);
	}

	function pop()
	{
		return $this->remote_takeout('pop');
	}

	function shift()
	{
		return $this->remote_takeout('shift');
	}
}

// ---------------------------------------------------------------------------
//

class SFQueue
{
	static function newClient($params=[])
	{
		if (isset($params['host']))
		{
			// connect remote host

			return new SFQueueClientRemote($params, true);
		}
		else
		{
			// connect localhost

			return new SFQueueClientLocal($params, true);
		}
	}

	static function newClientNotThrow($params=[])
	{
		if (isset($params['host']))
		{
			// connect remote host

			return new SFQueueClientRemote($params, false);
		}
		else
		{
			// connect localhost

			return new SFQueueClientLocal($params, false);
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

			var_dump($sfqc->push_text(['payload'=>'1234567890', 'payload_size'=>'5']));
			$sres();

			var_dump($sfqc->pop());
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
				'pop'		=> 12711,
				'shift'		=> 12721,
			],
			'timeout' =>
			[
				'read'		=> [ 'sec'=>2, 'usec'=>0 ],
				'write'		=> [ 'sec'=>2, 'usec'=>0 ],
			],
		];

		$etest(SFQueue::newClient($copt));
		echo PHP_EOL;

	// remote (not throw exception)

		$test(SFQueue::newClientNotThrow($copt));
		echo PHP_EOL;
	}
}

// EOF

