using static SlimCS;
public static class Test
{
	static int a()
	{
		print("a");
		return 0;
	}
	static int b()
	{
		print("b");
		return 1;
	}

	[System.Diagnostics.Conditional("NDEBUG")]
	static void Print(int a, int b)
	{
		print(a, b);
	}

	public static void Main()
	{
		print("FIRST");
		Print(
			a(), /*Interior a*/
			b() /*Interior b*/);
		print("SECOND");
	}
}
