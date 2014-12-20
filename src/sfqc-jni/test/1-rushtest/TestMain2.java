package test;

import jp.co.iret.sfq.*;
import java.util.Map;
import java.util.HashMap;

public class TestMain2
{
	static java.io.PrintStream o = System.out;

	static boolean cont = true;
	static Object lock = new Object();

	static Thread startThread(final String quename)
	{
		Thread ret = new Thread()
		{
			public void run()
			{
				final String name = getName();

				Map<String, Object> params = new HashMap<String, Object>() {
				{
					put("metatext", name);
				}};

				try
				{
					SFQueueClientInterface sfqc = SFQueue.newClient(quename);

					params.put("payload", name);
					sfqc.push(params);
				}
				catch (Exception ex)
				{
					ex.printStackTrace();
				}
			}
		};

		ret.start();

		return ret;
	}

	static void doTest() throws Exception
	{
		Thread t1 = startThread(null);
		Thread t2 = startThread(null);
		Thread t3 = startThread("jni2");

		t1.join();
		t2.join();
		t3.join();
	}

	public static void main(String[] args)
	{
		try
		{
			doTest();
		}
		catch (Exception ex)
		{
			ex.printStackTrace();
		}
	}
}

