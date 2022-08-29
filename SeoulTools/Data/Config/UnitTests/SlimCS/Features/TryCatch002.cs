using static SlimCS;
public static class Test
{
	public static int Main()
	{
		try
		{
			TestIt();
		}
		catch (System.Exception e)
		{
			print(e.Message);
			return 0;
		}

		return 1;
	}

	static void TestIt()
	{
		try
		{
			print("Here1.");

			int j = 0;
			for (int i = 0; i < 10; ++i)
			{
				try
				{
					if (i == 5 || i == 9)
					{
						throw new System.Exception(i.ToString() + "_" + j.ToString());
					}

					if (i == 9)
					{
						break;
					}
					else if (i % 2 == 0)
					{
						continue;
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
