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
		// Unsafe blocks are not supported.
		unsafe
		{
			int i = 1;
			int* p = &i;
			*p = 2;

			print(*p);
		}
	}
}
