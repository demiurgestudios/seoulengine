using static SlimCS;
using System;

public static class Test
{
	public static void Main()
	{
		int i = 0;
		void f(ref int j)
		{
			j = 5;
		}

		f(ref i);
		print(i);
	}
}
