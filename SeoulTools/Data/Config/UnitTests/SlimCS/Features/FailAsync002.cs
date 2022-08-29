using static SlimCS;
using System;
using System.Threading.Tasks;

// A Fail test (any test with a filename starting with Fail)
// tests that functionality which has been specifically disabled
// in SlimCS triggers a compiler failure.
public static class Test
{
	public static void Main()
	{
		async void AsyncOp()
		{
			for (int i = 0; i < 1000; ++i)
			{
				print("Hello World");
			}
		}
	}
}
