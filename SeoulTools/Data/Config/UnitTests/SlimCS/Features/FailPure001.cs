using System.Diagnostics.Contracts;

using static SlimCS;

public static class Test
{
	static double s_fValue = 1.0;

	// Pure method cannot assign to members.
	[Pure]
	public static void Pure()
	{
		s_fValue = 0.0;
	}

	public static void Main()
	{
		print("PURE TEST");
	}
}
