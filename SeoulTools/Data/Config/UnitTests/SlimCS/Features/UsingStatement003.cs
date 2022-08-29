using static SlimCS;
public static class Test
{
	class Foo : System.IDisposable
	{
		public readonly int m_i;

		public Foo(int i)
		{
			m_i = i;
		}

		public void Dispose()
		{
			print(m_i);
		}
	}

	public static int Main()
	{
		Foo a = new Foo(2);
		for (int j = 0; j < 10; ++j)
		{
			try
			{
				using (a)
				{
					print(1);

					if (a.m_i == 2)
					{
						break;
					}
				}
			}
			catch (System.Exception e)
			{
				print(e.Message);
			}
			finally
			{
				if (a.m_i == 2)
				{
					print("4");
				}
			}

			return 1;
		}

		return 0;
	}
}
