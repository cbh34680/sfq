using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

//
// http://baba-s.hatenablog.com/entry/2014/06/27/203952
//
public static class DictionaryExtensions
{
    /// <summary>
    /// 指定したキーに関連付けられている値を取得します。
    /// キーが存在しない場合は既定値を返します
    /// </summary>
    public static TValue GetOrDefault<TKey, TValue>(this Dictionary<TKey, TValue> self,
        TKey key, TValue defaultValue = default(TValue))
    {
        TValue value;
        return self.TryGetValue(key, out value) ? value : defaultValue;
    }

    public static String myToString<TValue>(this List<TValue> self, String separator=";")
    {
        string ret = "";

        foreach (TValue elem in self)
        {
            ret += elem.ToString() + separator;
        }

        return ret;
    }

    public static String myToString<TKey, TValue>(this Dictionary<TKey, TValue> self)
    {
        string ret = "";

        foreach (KeyValuePair<TKey, TValue> pair in self)
        {
            ret += pair.Key.ToString() + "=" + pair.Value.ToString() + ";";
        }

        return ret;
    }

    private static TRet returnDefault_<TRet>(Object o1, Object o2)
    {
        if (o1 is TRet)
        {
            return (TRet)o1;
        }

        if (o2 is TRet)
        {
            return (TRet)o2;
        }

        return default(TRet);
    }

    public static String s<TKey, TValue>(this Dictionary<TKey, TValue> self,
        TKey key, TValue defaultValue = default(TValue))
    {
        return returnDefault_<String>(self.GetOrDefault(key, defaultValue), defaultValue);
    }

    public static int i<TKey, TValue>(this Dictionary<TKey, TValue> self,
        TKey key, TValue defaultValue = default(TValue))
    {
        return returnDefault_<int>(self.GetOrDefault(key, defaultValue), defaultValue);
    }

    public static byte[] ba<TKey, TValue>(this Dictionary<TKey, TValue> self,
        TKey key, TValue defaultValue = default(TValue))
    {
        return returnDefault_<byte[]>(self.GetOrDefault(key, defaultValue), defaultValue);
    }
}

