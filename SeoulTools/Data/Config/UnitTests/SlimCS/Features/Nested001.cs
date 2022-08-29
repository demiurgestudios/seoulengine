using static SlimCS;
public static class Test
{
	static int a()
	{
		return 0;
	}

	public static void Main()
	{
		var i = a();
		print(i);
		{
			bool a = false;
			print(a);
		}
		i = a();
		print(i);
	}
}
