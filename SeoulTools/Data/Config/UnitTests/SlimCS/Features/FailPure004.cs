using System.Diagnostics.Contracts;

using static SlimCS;

public static class Test
{
	public static void Impure()
	{
	}

	// Pure method can only call pure methods.
	public static double Pure
	{
		[Pure]
		get { Impure(); return 0.0; }
	}

	public static void Main()
	{
		print("PURE TEST");
	}
}
