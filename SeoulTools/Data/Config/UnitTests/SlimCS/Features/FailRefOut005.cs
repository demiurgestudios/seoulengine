using static SlimCS;
using System;

public static class Test
{
	public static void Main()
	{
		int i = 0;
		B.Func f = (out int j) =>
		{
			j = 5;
		};

		f(out i);
		print(i);
	}
}

static class B
{
	delegate void Func(out int i);
}
