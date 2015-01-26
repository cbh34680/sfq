<?php

//
interface SFQueueClientInterface
{
	public function push_text($params);
	public function push_binary($params);
	public function pop($params);
	public function shift($params);
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
		1060	=> 'socket io error',
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
	private $lastError_ = 0;
	private $lastMessage_ = '';

	public function lastError($arg=null)
	{
		if (is_null($arg))
		{
			return $this->lastError_;
		}
		$this->lastError_ = $arg;
	}

	public function lastMessage($arg=null)
	{
		if (is_null($arg))
		{
			return $this->lastMessage_;
		}
		$this->lastMessage_ = $arg;
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
	private $nativeError_ = false;

	protected $quename_;
	private $querootdir_;

	function __construct($params, $throwException)
	{
		if (extension_loaded('sfqc_php'))
		{
			if (is_array($params))
			{
				$this->querootdir_ = @$params['querootdir'];
				$this->quename_ = @$params['quename'];
			}
		}
		else
		{
			$this->nativeError_ = true;

			throw new SFQueueClientException(1010);
		}
	}

// private:
	private function native_push($params)
	{
		$ret = false;

		$this->clearLastError();

		if ($this->nativeError_)
		{
			throw new SFQueueClientException(1020);
		}

		if (! is_array($params))
		{
			throw new SFQueueClientException(1040);
		}

		$rc = wrap_sfq_push($this->querootdir_, $this->quename_, $params);

		if ($rc == SFQ_RC_SUCCESS)
		{
			$ret = $params['uuid'];
		}
		else
		{
			$this->lastError($rc);
			$this->lastMessage("native error rc={$rc}");

			if ($rc >= SFQ_RC_FATAL_MIN)
			{
				throw new SFQueueClientException(2000 + $rc, "native error rc={$rc}");
			}
		}

		return $ret;
	}

	private function native_takeout($funcname)
	{
		$ret = false;

		$this->clearLastError();

		if ($this->nativeError_)
		{
			throw new SFQueueClientException(1020);
		}

		$params = [];
		$rc = wrap_sfq_takeout($this->querootdir_, $this->quename_, $params, $funcname);

		if ($rc == SFQ_RC_SUCCESS)
		{
			$ret = $params;
		}
		else
		{
			$this->lastError($rc);
			$this->lastMessage("native error rc={$rc}");

			if ($rc >= SFQ_RC_FATAL_MIN)
			{
				throw new SFQueueClientException(2000 + $rc, "native error rc={$rc}");
			}
		}

		return $ret;
	}

// public:
	function push_text($params)
	{
		if (isset($params['payload']))
		{
			$params['payload_type'] = 'text';
		}

		return $this->native_push($params);
	}

	function push_binary($params)
	{
		if (isset($params['payload']))
		{
			$params['payload_type'] = 'binary';
		}

		return $this->native_push($params);
	}

	function pop($params=[])
	{
		return $this->native_takeout('sfq_pop');
	}

	function shift($params=[])
	{
		return $this->native_takeout('sfq_shift');
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
				if ($addr == $arg)
				{
					// error
				}
				else
				{
					$ret = $addr;
				}
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
					if (! preg_match('/^\\s*([^:[:blank:]]+)\\s*:\\s*(.*)$/', trim($line), $matches))
					{
						continue;
					}

					list($all, $key, $val) = $matches;

					$key = str_replace('-', '_', trim($key));
					$header[$key] = trim($val);
				}
			}

			$body = substr($arg, $sep_offset + 4);
		}

		return [ 'header' => $header, 'body' => $body ];
	}

	function remote_io($opname, $params=[])
	{
		$ret = false;

		$connected = false;

		try
		{
			if (! isset($this->conn_params['host']))
			{
				throw new Exception("connection host is not set");
			}

			$host = $this->conn_params['host'];
			$port = @$this->conn_params['port'][$opname];

			if (! $port)
			{
				switch ($opname)
				{
					case 'push':	$port = 12701; break;
					case 'pop':		$port = 12711; break;
					case 'shift':	$port = 12721; break;

					default:
					{
						throw new Exception("{$opname}: unknown operation");
						break;
					}
				}
			}

			$to_read = @$this->conn_params['timeout']['read'];
			$to_write = @$this->conn_params['timeout']['write'];

			$to_read || $to_read = [ 'sec'=>2, 'usec'=>0 ];
			$to_write || $to_write = [ 'sec'=>2, 'usec'=>0 ];

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

			//
			$sock = @socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
			if ($sock === false)
			{
				throw new Exception('socket_create');
			}

			$t = @socket_connect($sock, $addr, $port);
			if ($t === false)
			{
				throw new Exception('socket_connect');
			}
			$connected = true;

			socket_set_option($sock, SOL_SOCKET, SO_RCVTIMEO, $to_read);
			socket_set_option($sock, SOL_SOCKET, SO_SNDTIMEO, $to_write);

			$t = @socket_write($sock, $header, strlen($header));
			if ($t === false)
			{
				throw new Exception('socket_write');
			}

			$sep = self::CRLF . self::CRLF;
			$t = @socket_write($sock, $sep, strlen($sep));
			if ($t === false)
			{
				throw new Exception('socket_write');
			}

			if (isset($body))
			{
				$t = @socket_write($sock, $body, strlen($body));
				if ($t === false)
				{
					throw new Exception('socket_write');
				}
			}

			$resp_text = '';
			do
			{
				$buff = @socket_read($sock, 8192);
				if ($buff !== false)
				{
					$resp_text .= $buff;
				}
			}
			while (! empty($buff));

			@socket_shutdown($sock, 2);
			@socket_close($sock);

			unset($sock);

			//
			$ret = self::parse_response($resp_text);
		}
		catch (Exception $ex)
		{
			if (isset($sock))
			{
				if ($connected)
				{
					@socket_shutdown($sock, 2);
				}
				@socket_close($sock);
			}

			throw new SFQueueClientException(1060);
		}

		return $ret;
	}

	private function isDataOrThrow($resp)
	{
		$ret = false;

		if (! isset($resp['header']['result_code']))
		{
			throw new SFQueueClientException(1050);
		}

		$rc = intval($resp['header']['result_code']);

		if ($rc == 0 /* SFQ_RC_SUCCESS */)
		{
			$ret = true;
		}
		else
		{
			$msg = "* Un-Known *";
			if (isset($resp['header']['error_message']))
			{
				$msg = $resp['header']['error_message'];
			}

			$this->lastError($rc);
			$this->lastMessage("remote error rc={$rc} msg={$msg}");

			if ($rc >= 21 /* SFQ_RC_FATAL_MIN */)
			{
				throw new SFQueueClientException(2000 + $rc, "remote error rc={$rc} msg={$msg}");
			}
		}

		return $ret;
	}

	private function remote_push($params)
	{
		$ret = false;

		$this->clearLastError();

		if (! is_array($params))
		{
			throw new SFQueueClientException(1040);
		}

		$resp = $this->remote_io('push', $params);
		if ($this->isDataOrThrow($resp))
		{
			$ret = $resp['header']['uuid'];
		}

		return $ret;
	}

	function remote_takeout($opname, $params)
	{
		$ret = false;

		$this->clearLastError();

		if (! is_array($params))
		{
			throw new SFQueueClientException(1040);
		}

		$resp = $this->remote_io($opname, $params);
		if ($this->isDataOrThrow($resp))
		{
			$ret = $resp['header'];
			$ret['payload'] = $resp['body'];
		}

		return $ret;
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

	function pop($params=[])
	{
		return $this->remote_takeout('pop', $params);
	}

	function shift($params=[])
	{
		return $this->remote_takeout('shift', $params);
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
	}
}

// EOF

