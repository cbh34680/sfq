package jp.co.iret.sfq;

import java.util.HashMap;

public class SFQueue
{
	static SFQueueClientInterface newClient()
	{
		return new newClient(new HashMap<String, Object>(), null);
	}

	static SFQueueClientInterface newClient(Map<String, Object> params)
	{
		return new newClient(params, null);
	}

	static SFQueueClientInterface newClient(Map<String, Object> params, String quename)
	{
		if (params.containsKey("host"))
		{
			return null;
		}
		else
		{
			return new SFQueueClientLocal(quename, params
		}
	}

	public static void main(String[] args)
	{
	}
}

