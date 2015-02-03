using sfq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TestConsole
{
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

                Console.WriteLine("push start");

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

                Console.WriteLine(String.Format("push(string) --> uuid=[{0}] (req={1])", sfqc.push(params1), params1));
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
        }
    }
}
