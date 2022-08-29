using static SlimCS;
public static class Test
{
	public static void Main()
	{
		TestIt();
	}

	public static void TestIt()
	{
		int? value = 0;
		int? res = value?.TestDo();
		print(res);
	}

	public static int TestDo(this int value)
	{
		print(value);
		return 1;
	}
}
