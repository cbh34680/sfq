using sfq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TestConsole
{
    using ObjectMap = Dictionary<String, Object>;

    class Program
    {
        static void Main(string[] args)
        {
            var params1 = new Dictionary<String, Object>()
                {
                    { "metatext", "params1" },
                    { "payload",  "string test" },
                };

            Console.WriteLine(String.Format("{0}", params1.myToString()));

            int x = default(int);

            qremote();
        }

        delegate void print_type(String format, params Object[] arg);
        private static print_type print = Console.WriteLine;
        
        private static void qremote()
        {
            try
            {
                var copt = new Dictionary<String, Object>()
                {
                    { "host", "sfq.nodanet" },
                    { "port", new Dictionary<String, int>()
                        {
                            { "push", 12701 },
                            { "pop", 12711 },
                            { "shift", 12721 },
                        }
                    },

                    { "timeout", 2000 },
                    { "querootdir", "/home/devuser/rq0" },
                    { "quename", "test0" },
                    { "eworkdir", "/home/devuser/rq0/w" },
                };

                SFQueueClientInterface sfqc = SFQueue.newClient(copt);

                print("push start");

                var params1 = new Dictionary<String, Object>()
                {
                    { "metatext", "params1" },
                    { "payload",  "string test" },
                };

                var params2 = new Dictionary<String, Object>()
                {
                    { "metatext", "params2" },
                    { "payload",  Encoding.UTF8.GetBytes("string to byte-array test") },
                };

                print("push(string) --> uuid=[{0}] (req={1})", sfqc.push(params1), params1.myToString());
                print("push(string) --> uuid=[{0}] (req={1})", sfqc.push(params2), params2.myToString());

                ObjectMap popv = sfqc.pop();
                ObjectMap shiftv = sfqc.shift();

                print("pop()   --> {0} (resp={1})", popv.myToString(), Encoding.UTF8.GetString(popv.ba("payload")));
                print("shift() --> {0}", shiftv.myToString());
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
        }
    }
}
