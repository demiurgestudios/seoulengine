using static SlimCS;
using System;

class Test
{
	// This tests a regression
	public static void Main ()
	{
		object n = null;
		try { print((string)n); } catch (System.Exception e) { print(e.Message); }
	}
}
