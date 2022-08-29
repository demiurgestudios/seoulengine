using static SlimCS;
using System;
using System.Threading.Tasks;

// Regression for a case where the loop variable
// of an "expensive" for loop leaked into the global
// table.
public static class Test
{
	public static void Main()
	{
		print(_G.i);
		var a = new int[] { 1, 2, 3, 4, 5 };
		for (int i = 0; 5 != a[i]; i++)
		{
			print(i);
			print(a[i]);
		}
		print(_G.i);
	}
}
