using static SlimCS;
public static class Test
{
	// Regression test for a bug where the SlimCS
	// compiler was not emitting explicit blocks
	// of case statements inside a switch block.
	public static void Main()
	{
		double a = 5.0;
		double b = 7.0;
		for (int i = 0; i < 3; ++i)
		{
			switch (i)
			{
				case 0:
				{
					var c = a + b;
					b = c;
				}
				break;
				case 1:
				{
					var c = a - b;
					a = c;
				}
				break;
				case 2:
				{
					var c = a * b;
					b = c;
				}
				break;
				default:
				{
					var c = a / b;
					a = c;
				}
				break;
			}

			print(a, b);
			print(_G._);
		}
	}
}
