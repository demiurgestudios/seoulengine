using static SlimCS;
public static class Test
{
	public static void Main()
	{
		try
		{
			TestIt();
		}
		catch (System.Exception e)
		{
			print(e.Message);
		}
	}

	static void TestIt()
	{
		try
		{
			print("Here1.");
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
