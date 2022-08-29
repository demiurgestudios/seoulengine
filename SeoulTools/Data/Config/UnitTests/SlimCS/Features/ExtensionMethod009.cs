using static SlimCS;
using System;
using System.Collections.Generic;
using N2;

namespace N
{
	static class S
	{
		internal static void Map<T>(this int i, Func<T, string> f) where T : new()
		{
			print(i, f(new T()));
		}
	}
}

namespace N2
{
	static class S2
	{
		internal static void Map(this int i, int k)
		{
			print(i, k);
		}
	}
}

namespace M
{
	using N;

	static class Test
	{
		public static void Main ()
		{
			1.Map(2);
		}
	}
}
