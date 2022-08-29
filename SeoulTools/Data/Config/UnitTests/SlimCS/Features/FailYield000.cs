using static SlimCS;
using System;
using System.Threading.Tasks;

// A Fail test (any test with a filename starting with Fail)
// tests that functionality which has been specifically disabled
// in SlimCS triggers a compiler failure.
public static class Test
{
	public System.Collections.Generic.IEnumerable<double> Test()
	{
		for (int i = 0; i < 1000; ++i)
		{
			yield return (double)i;
		}
	}

	public static void Main()
	{
		foreach (var v in Test())
		{
			print(v);
		}
	}
}
