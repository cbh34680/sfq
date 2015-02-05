package jp.co.iret.sfq;

import java.util.HashMap;
import java.util.Map;

@SuppressWarnings("serial")
public class SFQueueClientException extends Exception
{
	static Map<Integer, String> mesgmap;

	static
	{
		mesgmap = new HashMap<Integer, String>()
		{
			{
				put(0,		"it has not yet been set");
				put(1010,	"extension(libsfqc-jni.so) is not loaded");
				put(1020,	"it has not yet been initialized");
				put(1030,	"it does not yet implemented");
				put(1040,	"illegal argument type exception");
				put(1050,	"unknown error");
				put(1060,	"socket io error");
				put(1070,	"resolv host-address error");

				put(3010,	"illegal type response exception");
			}
		};
	}

	static String c2m(final int code)
	{
		if (mesgmap.containsKey(code))
		{
			return mesgmap.get(code);
		}

		return "unknown code (" + code + ")";
	}

	SFQueueClientException(final int code)
	{
		this(code, c2m(code));
	}

	SFQueueClientException(final int code, final String mesg)
	{
		super(mesg);

		code_ = code;
	}

	private final int code_;

	public int getCode()
	{
		return code_;
	}
}
