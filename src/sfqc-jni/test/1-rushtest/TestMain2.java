package test;

import jp.co.iret.sfq.*;
import java.util.Map;
import java.util.HashMap;

public class TestMain2
{
	static java.io.PrintStream o = System.out;

	static boolean cont = true;
	static Object lock = new Object();

	static Thread startThread(final int type)
	{
		Thread ret = new Thread()
		{
			public void run()
			{
				final String name = getName();

o.println(name + "(" + type + "): start");

				Map<String, Object> params = new HashMap<String, Object>() {
				{
					put("metatext", name);
				}};

				try
				{
					SFQueueClientInterface sfqc = SFQueue.newClient();

o.println(name + "(" + type + "): before push");
					params.put("payload", name);
					//synchronized (lock)
					{
						sfqc.push(params);
					}
o.println(name + "(" + type + "): after push");
				}
				catch (Exception ex)
				{
					ex.printStackTrace();
				}

o.println(name + "(" + type + "): stop");
			}
		};

		ret.start();

		return ret;
	}

	static void doTest() throws Exception
	{
		Thread t1 = startThread(1);
		Thread t2 = startThread(1);
		Thread t3 = startThread(1);
		Thread t4 = startThread(2);
		Thread t5 = startThread(3);

		t1.join();
		t2.join();
		t3.join();
		t4.join();
		t5.join();
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

