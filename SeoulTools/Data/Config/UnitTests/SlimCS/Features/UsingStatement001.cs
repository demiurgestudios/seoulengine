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

	public static void Main()
	{
		Foo a = new Foo(2);
		try
		{
			using (a)
			{
				print(1);
				throw new System.Exception("3");
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
	}
}
