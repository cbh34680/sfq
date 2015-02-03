using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace sfq
{
    using ObjectMap = Dictionary<String, Object>;

    class SFQueueClientRemote : SFQueueClient
    {
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
            return null;
        }

        public override ObjectMap shift(ObjectMap param)
        {
            return null;
        }
    }
}
