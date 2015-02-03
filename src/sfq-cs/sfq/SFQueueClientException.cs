using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace sfq
{
    class SFQueueClientException : Exception
    {
        static Dictionary<int, String> mesgmap;

        static SFQueueClientException()
        {
            mesgmap = new Dictionary<int, String>()
            {
				{0,		"it has not yet been set"},
                {1010,	"extension(libsfqc-jni.so) is not loaded"},
                {1020,	"it has not yet been initialized"},
                {1030,	"it does not yet implemented"},
                {1040,	"illegal argument type exception"},
                {1050,	"unknown error"},
                {1060,	"socket io error"},
                {1070,	"resolv host-address error"},

                {3010,	"illegal type response exception"},
            };
        }

        static String c2m(int code)
        {
            if (mesgmap.ContainsKey(code))
            {
                return mesgmap[code];
            }

            return "unknown code (" + code + ")";
        }

        public SFQueueClientException(int code) : this(code, c2m(code)) { }

        public SFQueueClientException(int code, string mesg) : base(mesg)
        {
            code_ = code;
        }

        private int code_;

        public int Code
        {
            get
            {
                return code_;
            }
        }
    }
}
