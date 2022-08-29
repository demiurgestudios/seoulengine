using static SlimCS;
using System;

public static class Test
{
	static void A((double, double) a)
	{
		print(a.Item1, a.Item2);
	}

	public static void Main()
	{
		A((1.0, 2.0));
	}
}
