package jp.co.iret.sfq;

import java.util.HashMap;
import java.util.Map;

public class SFQueue
{
	static SFQueueClientInterface newClient()
		throws SFQueueClientException
	{
		return newClient(null, new HashMap<String, Object>());
	}

	static SFQueueClientInterface newClient(Map<String, Object> params)
		throws SFQueueClientException
	{
		return newClient(null, params);
	}

	static SFQueueClientInterface newClient(String quename)
		throws SFQueueClientException
	{
		return newClient(quename, null);
	}

	static SFQueueClientInterface newClient(String quename, Map<String, Object> params)
		throws SFQueueClientException
	{
		if (params.containsKey("host"))
		{
			return null;
		}
		else
		{
			return new SFQueueClientLocal(quename, params, true);
		}
	}

	static java.io.PrintStream o = System.out;

	public static void main(String[] args)
	{
		try
		{
			SFQueueClientInterface sfqc = SFQueue.newClient();

o.println("push start");

			Map<String, Object> params1 = new HashMap<String, Object>()
			{
				{
					put("metatext", "params1");
					put("payload", "string test");
				}
			};

			Map<String, Object> params2 = new HashMap<String, Object>()
			{
				{
					put("metatext", "params2");
					put("payload", "string to byte-array test".getBytes("UTF-8"));
				}
			};

			o.printf("push(string) --> uuid=[%s] (req=%s)\n", sfqc.push(params1), params1);
			o.printf("push(binary) --> uuid=[%s] (req=%s)\n", sfqc.push(params2), params2);

			Map<String, Object> popv = sfqc.pop();
			Map<String, Object> shiftv = sfqc.shift();

			o.printf("pop()   --> %s (resp=%s)\n", popv, new String((byte[])popv.get("payload"), "UTF-8"));
			o.printf("shift() --> %s\n", shiftv);

o.println("push end");

		}
		catch (SFQueueClientException ex)
		{
			o.printf("CODE=%d\n", ex.getCode());
			o.printf("MESG=%s\n", ex.getMessage());

			ex.printStackTrace();
		}
		catch (Exception ex)
		{
			ex.printStackTrace();
		}
	}
}
