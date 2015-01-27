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
		catch (final Throwable ta)
		{
final String javaLibPath = System.getProperty("java.library.path");
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

	SFQueueClientLocal(final Map<String, Object> params)
		throws SFQueueClientException
	{
		if (! dll_loaded)
		{
			nativeError_ = true;

			throw new SFQueueClientException(1010, ta_.getMessage());
		}

		//
		querootdir_ = (String)params.get("querootdir");
		quename_ = (String)params.get("quename");
	}

	@Override
	public String push(final Map<String, Object> arg) throws SFQueueClientException
	{
		String ret = null;

		clearLastError();

		if (nativeError_)
		{
			throw new SFQueueClientException(1020);
		}

		final HashMap<String, Object> params = new HashMap<>(arg);
		final int rc = wrap_sfq_push(querootdir_, quename_, params);

		if (rc == SFQ_RC_SUCCESS)
		{
			final Object uuid = params.get("uuid");
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

	private Map<String, Object> native_takeout(final String funcname) throws SFQueueClientException
	{
		Map<String, Object> ret = null;

		clearLastError();

		if (nativeError_)
		{
			throw new SFQueueClientException(1020);
		}

		final HashMap<String, Object> params = new HashMap<>();
		final int rc = warp_sfq_takeout(querootdir_, quename_, params, funcname);

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
	public Map<String, Object> pop(final Map<String, Object> param) throws SFQueueClientException
	{
		return native_takeout("sfq_pop");
	}

	@Override
	public Map<String, Object> shift(final Map<String, Object> param) throws SFQueueClientException
	{
		return native_takeout("sfq_shift");
	}
}
