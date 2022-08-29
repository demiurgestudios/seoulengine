using static SlimCS;
public static class Test
{
	public delegate void T();
	public static void Main()
	{
		for (int i = 0; i < 10; ++i)
		{
			print('i', i);
			if (i % 2 == 0)
			{
				T f = () =>
				{
					for (int j = 10; j >= 0; --j)
					{
						print('j', i);
						if (j % 2 != 0)
						{
							continue;
						}

						++i;
					}
				};
				f();
				continue;
			}

			++i;
		}
	}
}
