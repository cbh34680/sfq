package test;

import jp.co.iret.sfq.*;
import java.util.Map;
import java.util.HashMap;

public class TestMain3
{
	static java.io.PrintStream o = System.out;

	static boolean cont = true;

	static Thread startThread(final String quename, final int type)
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
					SFQueueClientInterface sfqc = SFQueue.newClient(quename);

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
o.printf("JAVA LOOP %d\n", i);

			cont = true;

			Thread t11 = startThread("jni1", 1);
			Thread t12 = startThread("jni1", 1);
			Thread t13 = startThread("jni1", 1);
			Thread t14 = startThread("jni1", 2);
			Thread t15 = startThread("jni1", 3);

			Thread t21 = startThread("jni2", 1);
			Thread t22 = startThread("jni2", 1);
			Thread t23 = startThread("jni2", 1);
			Thread t24 = startThread("jni2", 2);
			Thread t25 = startThread("jni2", 3);

			Thread.sleep(10 * 1000);

			cont = false;

			t11.join();
			t12.join();
			t13.join();
			t14.join();
			t15.join();

			t21.join();
			t22.join();
			t23.join();
			t24.join();
			t25.join();

			t11 = null;
			t12 = null;
			t13 = null;
			t14 = null;
			t15 = null;

			t21 = null;
			t22 = null;
			t23 = null;
			t24 = null;
			t25 = null;
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

