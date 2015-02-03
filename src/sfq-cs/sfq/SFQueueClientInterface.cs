using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace sfq
{
    using ObjectMap = Dictionary<String, Object>;

    public interface SFQueueClientInterface
    {
        String push(ObjectMap param);
        ObjectMap pop(ObjectMap param);
        ObjectMap shift(ObjectMap param);
        ObjectMap pop();
        ObjectMap shift();
    }
}
