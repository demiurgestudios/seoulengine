using static SlimCS;
public static class Test
{
	static void TestIt(int a = 1, bool b = true, double f = 5.5)
	{
		print(a, b, f);
	}

	public static void Main()
	{
		TestIt();
		TestIt(0);
		TestIt(0, false);
		TestIt(0, false, 0.0);
	}
}
