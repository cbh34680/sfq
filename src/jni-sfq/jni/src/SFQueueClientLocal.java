package jp.co.iret.sfq;

import java.util.HashMap;
import java.util.Map;

public class SFQueueClientLocal extends SFQueueClient
{
	private native String wrap_sfq_push(Map<String, Object> params);
	private native Map<String, Object> warp_sfq_takeout(String funcname);

	static
	{
		try
		{
			System.loadLibrary("wrap_sfq");

			dll_loaded = true;
		}
		catch (Throwable ta)
		{
			dll_loaded = false;
		}

	};

	static boolean dll_loaded;

	String querootdir_;

	SFQueueClientLocal(String quename, Map<String, Object> params, boolean throwException)
		throws SFQueueClientException
	{
		super(quename, throwException);

		if (dll_loaded)
		{
			if (params.containsKey("querootdir"))
			{
				querootdir_ = (String)params.get("querootdir");
			}
		}
		else
		{
			throwException(1010);
		}
	}

	public String push_text(Map<String, Object> params)
	{
		return null;
	}

	public String push_binary(Map<String, Object> params)
	{
		return null;
	}

	public Map<String, Object> pop()
	{
		return new HashMap<String, Object>();
	}

	public Map<String, Object> shift()
	{
		return new HashMap<String, Object>();
	}

}

