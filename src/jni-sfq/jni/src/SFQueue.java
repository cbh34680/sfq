package jp.co.iret.sfq;

import java.util.HashMap;
import java.util.Map;

public class SFQueue
{
	static SFQueueClientInterface newClient()
		throws SFQueueClientException
	{
		return newClient(new HashMap<String, Object>(), null);
	}

	static SFQueueClientInterface newClient(Map<String, Object> params)
		throws SFQueueClientException
	{
		return newClient(params, null);
	}

	static SFQueueClientInterface newClient(Map<String, Object> params, String quename)
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

			sfqc.pop();

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

