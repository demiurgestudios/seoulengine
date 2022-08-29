using static SlimCS;
public static class Test
{
	// Regression test - tuple handling code may generate
	// a new variable, which resulted in
	// a "jumps into scope of local '_'" error at runtime.
	public static void Main()
	{
		double a = 5.0;
		double b = 7.0;
		for (int i = 0; i < 3; ++i)
		{
			switch (i)
			{
				case 0: (a, _) = (0.0, 1.0); break;
				case 1: (_, b) = (1.0, 2.0); break;
				case 2: // fall-through
				default: (a, b) = (3.0, 4.0); break;
			}

			print(a, b);
			print(_G._);
		}
	}
}
