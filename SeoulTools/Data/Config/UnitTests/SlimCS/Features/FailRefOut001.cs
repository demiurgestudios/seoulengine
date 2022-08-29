using static SlimCS;
using System;

public static class Test
{
	static void A(out int i)
	{
		i = 2;
	}

	public static void Main()
	{
		int i = 0;
		A(out i);
		print(i);
	}
}
