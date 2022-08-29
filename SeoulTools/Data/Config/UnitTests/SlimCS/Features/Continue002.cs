using static SlimCS;
public static class Test
{
	public static void Main()
	{
		for (int i = 0; i < 10; ++i)
		{
			print('i', i);
			if (i % 2 == 0)
			{
				continue;
			}

			var j = 0;
			while (j < 10)
			{
				print('j', j);
				++j;
				if (j == 4) { continue; }
				++j;
			}

			for (int k = 0; k < 10; ++k)
			{
				print('k', k);
				if (k % 2 != 0)
				{
					continue;
				}

				++k;
			}
		}
	}
}
