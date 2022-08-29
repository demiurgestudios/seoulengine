using static SlimCS;
public static class Test
{
	public static int Main()
	{
		try
		{
			return TestIt();
		}
		catch (System.Exception e)
		{
			print(e.Message);
			return 2;
		}

		return 1;
	}

	static int TestIt()
	{
		try
		{
			print("Here1.");

			int j = 0;
			for (int i = 0; i < 10; ++i)
			{
				try
				{
					if (i == 9)
					{
						return 0;
					}
					else if (i % 2 == 0)
					{
						continue;
					}

					if (i == 5 || i == 9)
					{
						throw new System.Exception(i.ToString() + "_" + j.ToString());
					}
				}
				catch (System.Exception e)
				{
					print(e.Message);

					if (i == 5)
					{
						continue;
					}
					else
					{
						break;
					}
				}

				++j;
			}

			throw new System.Exception("Here4.");
		}
		catch
		{
			try
			{
				print("Here2.");
				throw;
			}
			finally
			{
				print("Here3.");
			}
		}
	}
}
