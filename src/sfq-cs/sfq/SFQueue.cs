using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace sfq
{
    using ObjectMap = Dictionary<String, Object>;

    public class SFQueue
    {
        public static SFQueueClientInterface newClient()
        {
            return newClient(new ObjectMap());
        }

        public static SFQueueClientInterface newClient(ObjectMap param)
        {
            if (param.ContainsKey("host"))
            {
                return new SFQueueClientRemote(param);
            }

            return null;
        }
    }
}
