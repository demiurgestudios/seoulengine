using static SlimCS;
public static class Test
{
	class Foo : System.IDisposable
	{
		readonly int m_i;

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
		try
		{
			using (Foo a = new Foo(2), b = new Foo(1))
			{
				print(3);
				throw new System.Exception("4");
			}
		}
		catch (System.Exception e)
		{
			print(e.Message);
		}
	}
}
