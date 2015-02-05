using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace sfq
{
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Text.RegularExpressions;
    using ObjectMap = Dictionary<String, Object>;

    class SFQueueClientRemote : SFQueueClient
    {
        const string CRLF = "\r\n";
        const string LF = "\n";

        ObjectMap conn_params;

        public SFQueueClientRemote(ObjectMap param)
        {
            if (param == null)
            {
                throw new SFQueueClientException(1040);
            }

            conn_params = param;
        }

        private ObjectMap remote_io(String opname, ObjectMap param)
        {
            var ret = new ObjectMap();

            try
            {
                string host = (string)conn_params["host"];

                var portMap = (Dictionary<String, int>)conn_params.GetOrDefault("port", new Dictionary<String, int>());

                int port = portMap.GetOrDefault(opname);

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

                int timeout = conn_params.i("timeout", 2000);

                //
                var header_arr = new List<String>();
                var copyKeys = new List<String>() { "querootdir", "quename", "eworkdir" };
                var ignoreKeys = new List<String>() { "payload" };

                //
                byte[] body = null;

                if (param.ContainsKey("payload"))
                {
                    Object o = param["payload"];

                    if (o is String)
                    {
                        body = Encoding.UTF8.GetBytes((String)o);

                        header_arr.Add("payload-type: text");
                    }
                    else
                    {
                        body = (byte[])o;

                        header_arr.Add("payload-type: binary");
                    }

                    header_arr.Add("payload-length: " + body.Length);
                }

                //
                header_arr.AddRange
                (
                    conn_params.Keys
                        .Where(e => copyKeys.Contains(e))
                        .Select(e => e.Replace("_", "-") + ": " + conn_params.s(e))
                );

                header_arr.AddRange
                (
                    param.Keys
                        .Where(e => !ignoreKeys.Contains(e))
                        .Select(e => e.Replace("_", "-") + ": " + param.s(e))
                );

                //header_arr.ForEach(Console.WriteLine);

                byte[] respAll = null;

                string header = string.Join(CRLF, header_arr) + CRLF + CRLF;
                using (Socket sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
                {
                    sock.NoDelay = true;

                    IPHostEntry hostent = Dns.GetHostEntry(host);
                    sock.Connect(new System.Net.IPEndPoint(hostent.AddressList[0], port));

                    sock.ReceiveTimeout = timeout;
                    sock.SendTimeout = timeout;

                    sock.Send(Encoding.UTF8.GetBytes(header));

                    if (body != null)
                    {
                        sock.Send(body);
                    }

                    using (MemoryStream ms = new MemoryStream())
                    {
                        using (BinaryWriter bw = new BinaryWriter(ms))
                        {
                            byte[] buff = new byte[8192];

                            int rb = 0;
                            do
                            {
                                rb = sock.Receive(buff);
                                if (rb > 0)
                                {
                                    bw.Write(buff, 0, rb);
                                }
                            }
                            while (rb != 0);
                        }

                        respAll = ms.ToArray();
                    }

                    ret = parse_response(respAll);
                }
            }
            catch (SFQueueClientException ex)
            {
                throw ex;
            }
            catch (Exception ex)
            {
                throw new SFQueueClientException(1050, ex.Message);
            }

            return ret;
        }

        static ObjectMap parse_response(byte[] arg)
        {
            var ret = new ObjectMap();

            byte[] SEP_CRLF = Encoding.UTF8.GetBytes(CRLF + CRLF);
            byte[] SEP_LF = Encoding.UTF8.GetBytes(LF + LF);

            int sep_offset = indexOf_byteArray(arg, SEP_CRLF);
            int sep_length = SEP_CRLF.Length;

            if (sep_offset == -1)
            {
                sep_offset = indexOf_byteArray(arg, SEP_LF);
                sep_length = SEP_LF.Length;
            }

            byte[] baBody = null;
            bool bodyIsText = false;

            if (sep_offset == -1)
            {
                // body only
                baBody = arg;
            }
            else
            {
                if (sep_offset > 0)
                {
                    byte[] baHeader = arg.Take(sep_offset).ToArray();
                    string strHeader = Encoding.UTF8.GetString(baHeader);
                    string[] strsHeader = strHeader.Split(LF[0]);

                    foreach (string str in strsHeader)
                    {
                        MatchCollection matches = Regex.Matches(str, @"^\s*([^:[:blank:]]+)\s*:\s*(.*)$");

                        foreach (Match match in matches)
                        {
                            string[] sarr = match.Value.Split(':');
                            string key = sarr[0].Trim().Replace("-", "_");
                            string val = sarr[1].Trim();

                            if (key == "payload_type")
                            {
                                if (val == "text")
                                {
                                    bodyIsText = true;
                                }
                            }

                            ret[key] = val;

                            break;
                        }
                    }
                }

                baBody = arg.Skip(sep_offset + sep_length).ToArray();
            }

            if (baBody != null)
            {
                if (bodyIsText)
                {
                    ret["payload"] = Encoding.UTF8.GetString(baBody);
                }
                else
                {
                    ret["payload"] = baBody;
                }
            }

            return ret;
        }

        //
        // http://stackoverflow.com/questions/21341027/find-indexof-a-byte-array-within-another-byte-array
        //
        static int indexOf_byteArray(byte[] outerArray, byte[] innerArray)
	    {
		    for(int i = 0; i < outerArray.Length - innerArray.Length; ++i)
		    {
			    bool found = true;
			    for(int j = 0; j < innerArray.Length; ++j)
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

        private bool isDataOrThrow(ObjectMap resp)
        {
            bool ret = false;

            if (! resp.ContainsKey("result_code"))
            {
                throw new SFQueueClientException(1050);
            }

            int rc = int.Parse(resp.s("result_code"));

            if (rc == SFQ_RC_SUCCESS)
            {
                ret = true;
            }
            else
            {
                string msg = "* Un-Known *";
                if (resp.ContainsKey("error_message"))
                {
                    msg = resp.s("error_message");
                }

                LastError = rc;
                LastMessage = "remote error rc=" + rc + " msg=" + msg;

                if (rc >= SFQ_RC_FATAL_MIN)
                {
                    throw new SFQueueClientException(2000 + rc, "remote error rc=" + rc+ " msg=" + msg);
                }
            }

            return ret;
        }

        public override String push(ObjectMap param)
        {
            string ret = null;

            clearLastError();

            ObjectMap resp = remote_io("push", param);
            if (isDataOrThrow(resp))
            {
                ret = resp.s("uuid");
            }

            return ret;
        }

        private ObjectMap remote_takeout(string opname, ObjectMap param)
        {
            ObjectMap ret = null;

            clearLastError();

            var resp = remote_io(opname, param);
            if (isDataOrThrow(resp))
            {
                ret = resp;
            }

            return ret;
        }

        public override ObjectMap pop(ObjectMap param)
        {
            return remote_takeout("pop", param);
        }

        public override ObjectMap shift(ObjectMap param)
        {
            return remote_takeout("shift", param);
        }
    }
}
