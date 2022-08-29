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
		// Increment and decrement can only appear in
		// statements contexts in SlimCS, not as
		// expressions.
		var i = 0;
		i = i--;
	}
}
