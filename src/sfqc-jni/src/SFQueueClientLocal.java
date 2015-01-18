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

	String querootdir_;
	String quename_;

	SFQueueClientLocal(Map<String, Object> params, boolean throwException)
		throws SFQueueClientException
	{
		super(throwException);

		if (dll_loaded)
		{
			querootdir_ = (String)params.get("querootdir");
			quename_ = (String)params.get("quename");
		}
		else
		{
			throwException(1010, ta_.getMessage());
		}
	}

	public String push(Map<String, Object> arg) throws SFQueueClientException
	{
		clearLastError();

		if (notInitialized_)
		{
			throwException(1020);
		}
		else
		{
			HashMap<String, Object> params = new HashMap<>(arg);

			int rc = wrap_sfq_push(querootdir_, quename_, params);

			if (rc == SFQ_RC_SUCCESS)
			{
				Object uuid = params.get("uuid");
				if (uuid instanceof String)
				{
					return (String)uuid;
				}

				throwException(3010);
			}
			else if (rc > SFQ_RC_FATAL_MIN)
			{
				throwException(2000 + rc, "native error rc={" + rc + "}");
			}
		}

		return null;
	}

	private Map<String, Object> native_takeout_(String funcname) throws SFQueueClientException
	{
		clearLastError();

		if (notInitialized_)
		{
			throwException(1020);
		}
		else
		{
			HashMap<String, Object> params = new HashMap<>();

			int rc = warp_sfq_takeout(querootdir_, quename_, params, funcname);

			if (rc == SFQ_RC_SUCCESS)
			{
				return params;
			}
			else if (rc > SFQ_RC_FATAL_MIN)
			{
				throwException(2000 + rc, "native error rc={" + rc + "}");
			}
		}

		return null;
	}

	public Map<String, Object> pop() throws SFQueueClientException
	{
		return native_takeout_("sfq_pop");
	}

	public Map<String, Object> shift() throws SFQueueClientException
	{
		return native_takeout_("sfq_shift");
	}

}

