using static SlimCS;
using System;

public static class Test
{
	public static void Main()
	{
		(double, double) a = (1.0, 2.0);

		print(a.Item1, a.Item2);
	}
}
