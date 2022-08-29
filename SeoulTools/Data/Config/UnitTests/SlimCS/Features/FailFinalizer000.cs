using static SlimCS;
public static class Test
{
	class A
	{
		public A()
		{
			print("Construct");
		}

		~A()
		{
			print("Finalize");
		}
	}

	public static void Main()
	{
		new A();
	}
}
