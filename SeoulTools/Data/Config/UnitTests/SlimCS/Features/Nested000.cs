using static SlimCS;
public static class Test
{
	class a
	{
		public static int b = 0;
	}

	public static void Main()
	{
		{
			bool a = false;
			print(a);
		}
		var i = a.b;
		print(i);
	}
}
