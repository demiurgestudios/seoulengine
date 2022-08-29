using static SlimCS;
public static class Test
{
	static bool a = true;

	public static void Main()
	{
		print(a);
		if (true)
		{
			const int a = 0;
			print(a);
		}
		a = false;
		print(a);
	}
}
