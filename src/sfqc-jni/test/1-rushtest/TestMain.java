package test;

import jp.co.iret.sfq.*;
import java.util.Map;
import java.util.HashMap;

public class TestMain
{
	static java.io.PrintStream o = System.out;

	static boolean cont = true;

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

					for (int i=0; cont; i++)
					{
						switch (type)
						{
							case 1:
							{
								params.put("payload", "loop-" + i);
								sfqc.push(params);
								break;
							}
							case 2: sfqc.pop(); break;
							case 3: sfqc.shift(); break;
						}

						yield();
					}
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
		for (int i=0; true; i++)
		{
o.printf("BIG LOOP %d\n", i);

			cont = true;

			Thread t1 = startThread(1);
			Thread t2 = startThread(1);
			Thread t3 = startThread(1);
			Thread t4 = startThread(2);
			Thread t5 = startThread(3);

			Thread.sleep(20 * 1000);

			cont = false;

			t1.join();
			t2.join();
			t3.join();
			t4.join();
			t5.join();

			t1 = null;
			t2 = null;
			t3 = null;
			t4 = null;
			t5 = null;
		}
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

