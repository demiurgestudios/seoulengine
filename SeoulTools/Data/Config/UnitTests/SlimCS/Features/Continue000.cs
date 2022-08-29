using static SlimCS;
public static class Test
{
	public static void Main()
	{
		for (int i = 0; i < 100; ++i)
		{
			if (i % 2 == 0)
			{
				continue;
			}

			var a = i;
			print(a);
			print(i);
		}
	}
}
