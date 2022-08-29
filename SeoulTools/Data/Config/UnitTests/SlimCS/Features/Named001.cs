using static SlimCS;
using System;

class Test
{
	delegate int IntDelegate (int a, int b = 2);

	static int TestInt (int u, int v)
	{
		print(u, v);
		return 29;
	}

	public static int Main ()
	{
		var del = new IntDelegate (TestInt);
		del (a : 7);

		return 0;
	}
}
