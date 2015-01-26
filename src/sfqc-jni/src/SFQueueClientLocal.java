package jp.co.iret.sfq;

import java.util.HashMap;
import java.util.Map;

public class SFQueueClientLocal extends SFQueueClient
{
	private native int wrap_sfq_push(String querootdir, String quename,
		HashMap<String, Object> hashmap);

	private native int warp_sfq_takeout(String querootdir, String quename,
		HashMap<String, Object> hashmap, String takeoutfunc);

	static
	{
		try
		{
			System.loadLibrary("sfqc-jni");

			dll_loaded = true;
		}
		catch (Throwable ta)
		{
String javaLibPath = System.getProperty("java.library.path");
System.out.println(javaLibPath);
			ta_ = ta;
			dll_loaded = false;
		}

	};

	static boolean dll_loaded;
	static Throwable ta_;

	boolean nativeError_ = false;
	String querootdir_;
	String quename_;

	SFQueueClientLocal(Map<String, Object> params, boolean throwException)
		throws SFQueueClientException
	{
		if (dll_loaded)
		{
			querootdir_ = (String)params.get("querootdir");
			quename_ = (String)params.get("quename");
		}
		else
		{
			nativeError_ = true;

			throw new SFQueueClientException(1010, ta_.getMessage());
		}
	}

	@Override
	public String push(Map<String, Object> arg) throws SFQueueClientException
	{
		String ret = null;

		clearLastError();

		if (nativeError_)
		{
			throw new SFQueueClientException(1020);
		}

		HashMap<String, Object> params = new HashMap<>(arg);
		int rc = wrap_sfq_push(querootdir_, quename_, params);

		if (rc == SFQ_RC_SUCCESS)
		{
			Object uuid = params.get("uuid");
			if (! (uuid instanceof String))
			{
				throw new SFQueueClientException(3010);
			}

			ret = (String)uuid;
		}
		else
		{
			setLastError(rc);
			setLastMessage("native error rc={" + rc + "}");

			if (rc >= SFQ_RC_FATAL_MIN)
			{
				throw new SFQueueClientException(2000 + rc, "native error rc={" + rc + "}");
			}
		}

		return ret;
	}

	private Map<String, Object> native_takeout(String funcname) throws SFQueueClientException
	{
		Map<String, Object> ret = null;

		clearLastError();

		if (nativeError_)
		{
			throw new SFQueueClientException(1020);
		}

		HashMap<String, Object> params = new HashMap<>();
		int rc = warp_sfq_takeout(querootdir_, quename_, params, funcname);

		if (rc == SFQ_RC_SUCCESS)
		{
			ret = params;
		}
		else
		{
			setLastError(rc);
			setLastMessage("native error rc={" + rc + "}");

			if (rc >= SFQ_RC_FATAL_MIN)
			{
				throw new SFQueueClientException(2000 + rc, "native error rc={" + rc + "}");
			}
		}

		return ret;
	}

	@Override
	public Map<String, Object> pop(Map<String, Object> param) throws SFQueueClientException
	{
		return native_takeout("sfq_pop");
	}

	@Override
	public Map<String, Object> shift(Map<String, Object> param) throws SFQueueClientException
	{
		return native_takeout("sfq_shift");
	}
}
