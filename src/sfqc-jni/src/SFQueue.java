package jp.co.iret.sfq;

import java.util.HashMap;
import java.util.Map;

@SuppressWarnings("serial")
public class SFQueue
{
	public static SFQueueClientInterface newClient()
		throws SFQueueClientException
	{
		return newClient(new HashMap<String, Object>());
	}

	public static SFQueueClientInterface newClient(final Map<String, Object> params)
		throws SFQueueClientException
	{
		if (params.containsKey("host"))
		{
			return new SFQueueClientRemote(params);
		}
		else
		{
			return new SFQueueClientLocal(params);
		}
	}

	static java.io.PrintStream o = System.out;

	public static void main(final String[] args)
	{
		if (args.length > 0)
		{
			qremote();
		}
		else
		{
			qlocal();
		}
	}

	private static void qremote()
	{
		try
		{
			final Map<String, Object> copt = new HashMap<String, Object>()
			{
				{
					put("host", "sfq.nodanet");

					final Map<String, Integer> port = new HashMap<String, Integer>()
					{
						{
							put("push", 12701);
							put("pop", 12711);
							put("shift", 12721);
						}
					};

					put("port", port);

					put("querootdir", "/home/devuser/rq0");
					put("quename", "test0");
				}
			};

			final SFQueueClientInterface sfqc = SFQueue.newClient(copt);

o.println("push start");

			final Map<String, Object> params1 = new HashMap<String, Object>()
			{
				{
					put("eworkdir", "/home/devuser/rq0/w");

					put("metatext", "params1");
					put("payload", "string test");
				}
			};

			final Map<String, Object> params2 = new HashMap<String, Object>()
			{
				{
					put("metatext", "params2");
					put("payload", "string to byte-array test".getBytes("UTF-8"));
				}
			};

			o.printf("push(string) --> uuid=[%s] (req=%s)\n", sfqc.push(params1), params1);
			o.printf("push(binary) --> uuid=[%s] (req=%s)\n", sfqc.push(params2), params2);

			final Map<String, Object> popv = sfqc.pop();
			final Map<String, Object> shiftv = sfqc.shift();

			o.printf("pop()   --> %s (resp=%s)\n", popv, new String((byte[])popv.get("payload"), "UTF-8"));
			o.printf("shift() --> %s\n", shiftv);

o.println("push end");

		}
		catch (final SFQueueClientException ex)
		{
			o.printf("CODE=%d\n", ex.getCode());
			o.printf("MESG=%s\n", ex.getMessage());

			ex.printStackTrace();
		}
		catch (final Exception ex)
		{
			ex.printStackTrace();
		}
	}

	private static void qlocal()
	{
		try
		{
			final SFQueueClientInterface sfqc = SFQueue.newClient();

o.println("push start");

			final Map<String, Object> params1 = new HashMap<String, Object>()
			{
				{
					put("metatext", "params1");
					put("payload", "string test");
				}
			};

			final Map<String, Object> params2 = new HashMap<String, Object>()
			{
				{
					put("metatext", "params2");
					put("payload", "string to byte-array test".getBytes("UTF-8"));
				}
			};

			o.printf("push(string) --> uuid=[%s] (req=%s)\n", sfqc.push(params1), params1);
			o.printf("push(binary) --> uuid=[%s] (req=%s)\n", sfqc.push(params2), params2);

			final Map<String, Object> popv = sfqc.pop();
			final Map<String, Object> shiftv = sfqc.shift();

			o.printf("pop()   --> %s (resp=%s)\n", popv, new String((byte[])popv.get("payload"), "UTF-8"));
			o.printf("shift() --> %s\n", shiftv);

o.println("push end");

		}
		catch (final SFQueueClientException ex)
		{
			o.printf("CODE=%d\n", ex.getCode());
			o.printf("MESG=%s\n", ex.getMessage());

			ex.printStackTrace();
		}
		catch (final Exception ex)
		{
			ex.printStackTrace();
		}
	}
}
