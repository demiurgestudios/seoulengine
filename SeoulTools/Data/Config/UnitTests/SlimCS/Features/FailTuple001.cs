using static SlimCS;
using System;

public static class Test
{
	public static void Main()
	{
		B.Func f = ((double, double) a) =>
		{
			print(a.Item1, a.Item2);
		};

		f((1.0, 2.0));
	}
}

static class B
{
	delegate void Func((double, double) a);
}
