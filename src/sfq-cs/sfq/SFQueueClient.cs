using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace sfq
{
    using ObjectMap = Dictionary<String, Object>;

    abstract class SFQueueClient : SFQueueClientInterface
    {
        public abstract String push(ObjectMap param);
        public abstract ObjectMap pop(ObjectMap param);
        public abstract ObjectMap shift(ObjectMap param);

        protected const int SFQ_RC_SUCCESS   = 0;
        protected const int SFQ_RC_FATAL_MIN = 21;

        protected int LastError { set; get; }
        protected String LastMessage { set; get; }

        protected void clearLastError()
        {
            LastError = 0;
            LastMessage = "";
        }

        public ObjectMap pop()
        {
            return pop(new ObjectMap());
        }

        public ObjectMap shift()
        {
            return shift(new ObjectMap());
        }
    }
}
