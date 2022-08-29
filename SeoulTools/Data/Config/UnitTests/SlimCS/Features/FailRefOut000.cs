using static SlimCS;
using System;

public static class Test
{
	static void A(ref int i)
	{
		i = 2;
	}

	public static void Main()
	{
		int i = 0;
		A(ref i);
		print(i);
	}
}
