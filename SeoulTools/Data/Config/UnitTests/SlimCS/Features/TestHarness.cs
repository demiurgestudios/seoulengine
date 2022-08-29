using static SlimCS;
using static System.Console;
using System.Globalization;

public static class SlimCS
{
	static SlimCS()
	{
		// Ensure Unicode output is preserved.
		OutputEncoding = System.Text.Encoding.UTF8;
	}
	
	// Used for proper discard and other
	// checks, should never pollute
	// into _G._
	public static class _G
	{
		public static object _;
		public static object i;
	}

	public class Table
	{
		System.Collections.Generic.Dictionary<object, object> m_t = new System.Collections.Generic.Dictionary<object, object>();
		public object this[object key]
		{
			get
			{
				object ret = null;
				m_t.TryGetValue(key, out ret);
				return ret;
			}

			set
			{
				if (value == null)
				{
					m_t.Remove(key);
				}
				else
				{
					m_t[key] = value;
				}
			}
		}
	}

	public static string concat(params object[] args)
	{
		string s = "";
		foreach (var o in args)
		{
			s += tostring(o);
		}
		return s;
	}

	public static void print(params object[] a)
	{
		for (int i = 0; i < a.Length; ++i)
		{
			if (0 != i)
			{
				Write('\t');
			}

			var o = a[i];
			Write(tostring(o));
		}
		WriteLine();
	}

	public static string tostring(object o)
	{
		if (o is bool)
		{
			if ((bool)o == true) { return "true"; }
			else { return "false"; }
		}
		else if (o is char)
		{
			return ((int)((char)o)).ToString();
		}
		else if (null == o)
		{
			return "nil";
		}
		else if (o.GetType().IsEnum)
		{
			// TODO: Workaround for the fact that an implicit
			// tostring() conversion of an enum value is not yet supported.
			return ((int)o).ToString();
		}
		else
		{
			// Ensure Lua compatible print formatting invariant of system culture.
			return string.Format(CultureInfo.InvariantCulture, "{0}", o);
		}
	}

	public static class @string
	{
		public static string lower(string s)
		{
			return s.ToLower();
		}
	}
}
