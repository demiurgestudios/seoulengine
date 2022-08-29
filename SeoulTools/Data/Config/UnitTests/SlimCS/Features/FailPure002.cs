using System.Diagnostics.Contracts;

using static SlimCS;

public static class Test
{
	static double s_fValue1 = 1.0;
	static double s_fValue2 = 2.0;

	// Pure method cannot assign to members via tuple assignment.
	[Pure]
	public static void Pure()
	{
		(s_fValue1, s_fValue2) = (2.0, 1.0);
	}

	public static void Main()
	{
		print("PURE TEST");
	}
}
