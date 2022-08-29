using System.Diagnostics.Contracts;

using static SlimCS;

public static class Test
{
	public static void Impure()
	{
	}

	// Pure method can only call pure methods.
	[Pure]
	public static void Pure()
	{
		Impure();
	}

	public static void Main()
	{
		print("PURE TEST");
	}
}
