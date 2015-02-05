package jp.co.iret.sfq;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

public class SFQueueClientRemote extends SFQueueClient
{
	static final String CRLF = "\r\n";
	static final String LF = "\n";

	Map<String, Object> conn_params;

	SFQueueClientRemote(final Map<String, Object> params)
		throws SFQueueClientException
	{
		if (params == null)
		{
			throw new SFQueueClientException(1040);
		}

		conn_params = params;
	}

	private Map<String, Object> remote_io(final String opname, final Map<String, Object> params)
		throws SFQueueClientException
	{
		Map<String, Object> ret = null;

		try
		{
			final String host = (String)conn_params.get("host");

			@SuppressWarnings("unchecked")
			final
			//Map<String, Integer> portMap = (Map<String, Integer>)Optional.ofNullable(conn_params.get("port")).orElse(new HashMap<>());
			Map<String, Integer> portMap = (Map<String, Integer>)conn_params.getOrDefault("port", new HashMap<>());

			//int port = Optional.ofNullable(portMap.get(opname)).orElse(-1);
			int port = portMap.getOrDefault(opname, -1);

			if (port == -1)
			{
				switch (opname)
				{
					case "push":  { port = 12701; break; }
					case "pop":   { port = 12711; break; }
					case "shift": { port = 12721; break; }

					default:
					{
						throw new Exception(opname + ": unknown operation");
					}
				}
			}

			final int timeout = (int)conn_params.getOrDefault("timeout", 2000);

			//
			final ArrayList<String> header_arr = new ArrayList<>();
			final List<String> copyKeys = Arrays.asList("querootdir", "quename", "eworkdir");
			final List<String> ignoreKeys = Arrays.asList("payload");

			//
			byte[] body = null;

			if (params.containsKey("payload"))
			{
				final Object o = params.get("payload");

				if (o instanceof String)
				{
					body = ((String)o).getBytes();

					header_arr.add("payload-type: text");
				}
				else
				{
					body = (byte[])o;

					header_arr.add("payload-type: binary");
				}

				header_arr.add("payload-length: " + body.length);
			}

			//
			header_arr.addAll
			(
				conn_params.keySet().stream()
					.filter(e -> copyKeys.contains(e))
					.map(e -> e.replaceAll("_", "-") + ": " + (String)conn_params.get(e))
					.collect(Collectors.toList())
			);

			header_arr.addAll
			(
				params.keySet().stream()
					.filter(e -> ! ignoreKeys.contains(e))
					.map(e -> e.replaceAll("_", "-") + ": " + (String)params.get(e))
					.collect(Collectors.toList())
			);

			header_arr.forEach(System.out::println);

			//
			byte[] respAll = null;

			final String header = String.join(CRLF,  header_arr) + CRLF + CRLF;
			try (Socket sock = new Socket(host, port))
			{
				sock.setSoTimeout(timeout);

				try (OutputStream os = sock.getOutputStream(); DataInputStream dis = new DataInputStream(sock.getInputStream()))
				{
					os.write(header.getBytes());

					if (body != null)
					{
						os.write(body);
					}

					os.flush();

					final ByteArrayOutputStream baos = new ByteArrayOutputStream();
					final byte[] buff = new byte[8192];

					int rb = 0;
					do
					{
						rb = dis.read(buff);
						if (rb > 0)
						{
							baos.write(buff, 0, rb);
						}
					}
					while (rb != -1);

					respAll = baos.toByteArray();
				}
			}

			ret = parse_response(respAll);
		}
		catch (final SFQueueClientException ex)
		{
			throw ex;
		}
		catch (final Exception ex)
		{
			throw new SFQueueClientException(1050, ex.getClass().getName());
		}

		return ret;
	}

	static Map<String, Object> parse_response(final byte[] arg) throws Exception
	{
		final Map<String, Object> ret = new HashMap<>();

		final byte[] SEP_CRLF = (CRLF + CRLF).getBytes();
		final byte[] SEP_LF = (LF + LF).getBytes();

		int sep_offset = indexOf_byteArray(arg, SEP_CRLF);

		if (sep_offset == -1)
		{
			sep_offset = indexOf_byteArray(arg, SEP_LF);
		}

		byte[] baBody = null;
		boolean bodyIsText = false;

		if (sep_offset == -1)
		{
			// body only
			baBody = arg;
		}
		else
		{
			if (sep_offset > 0)
			{
				final byte[] baHeader = Arrays.copyOfRange(arg, 0, sep_offset);
				final String strHeader = new String(baHeader, "UTF-8");
				final String[] strsHeader = strHeader.split(LF);

				final Pattern pat = Pattern.compile("^\\s*([^:[:blank:]]+)\\s*:\\s*(.*)$");

				for (final String str: strsHeader)
				{
					final Matcher matches = pat.matcher(str.trim());

					if (! matches.find())
					{
						continue;
					}

					final String key = matches.group(1).trim().replaceAll("-",  "_");
					final String val = matches.group(2).trim();

					if (key.equals("payload_type"))
					{
						if (val.equals("text"))
						{
							bodyIsText = true;
						}
					}

					ret.put(key, val);
				}
			}

			baBody = Arrays.copyOfRange(arg, sep_offset + 4, arg.length);
		}

		if (baBody != null)
		{
			final Object oBody = bodyIsText ? new String(baBody, "UTF-8") : baBody;
			ret.put("payload", oBody);
		}

		return ret;
	}

	//
	// http://stackoverflow.com/questions/21341027/find-indexof-a-byte-array-within-another-byte-array
	//
	static int indexOf_byteArray(final byte[] outerArray, final byte[] innerArray)
	{
		for(int i = 0; i < outerArray.length - innerArray.length; ++i)
		{
			boolean found = true;
			for(int j = 0; j < innerArray.length; ++j)
			{
				if (outerArray[i+j] != innerArray[j])
				{
					found = false;
					break;
				}
			}
			if (found) return i;
		}
		return -1;
	}

	private boolean isDataOrThrow(final Map<String, Object> resp) throws SFQueueClientException
	{
		boolean ret = false;

		if (! resp.containsKey("result_code"))
		{
			throw new SFQueueClientException(1050);
		}

		final int rc = Integer.parseInt((String)resp.get("result_code"));

		if (rc == SFQ_RC_SUCCESS)
		{
			ret = true;
		}
		else
		{
			String msg = "* Un-Known *";
			if (resp.containsKey("error_message"))
			{
				msg = (String)resp.get("error_message");
			}

			setLastError(rc);
			setLastMessage("remote error rc=" + rc+ " msg=" + msg);

			if (rc >= SFQ_RC_FATAL_MIN)
			{
				throw new SFQueueClientException(2000 + rc, "remote error rc=" + rc+ " msg=" + msg);
			}
		}

		return ret;
	}

	@Override
	public String push(final Map<String, Object> arg) throws SFQueueClientException
	{
		String ret = null;

		clearLastError();

		final Map<String, Object> resp = remote_io("push", arg);
		if (isDataOrThrow(resp))
		{
			ret = (String)resp.getOrDefault("uuid", "");
		}

		return ret;
	}

	private Map<String, Object> remote_takeout(final String opname, final Map<String, Object> params)
		throws SFQueueClientException
	{
		Map<String, Object> ret = null;

		clearLastError();

		final Map<String, Object> resp = remote_io(opname, params);
		if (isDataOrThrow(resp))
		{
			ret = resp;
		}

		return ret;
	}

	@Override
	public Map<String, Object> pop(final Map<String, Object> params) throws SFQueueClientException
	{
		return remote_takeout("pop", params);
	}

	@Override
	public Map<String, Object> shift(final Map<String, Object> params) throws SFQueueClientException
	{
		return remote_takeout("shift", params);
	}
}
