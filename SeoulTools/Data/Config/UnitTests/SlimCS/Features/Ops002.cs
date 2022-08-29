using static SlimCS;
public static class Test
{
	public static void Main()
	{
		// Regression, unary minus on an integer should never
		// generate -0.
		var i = 0;
		print(-i);
	}
}
