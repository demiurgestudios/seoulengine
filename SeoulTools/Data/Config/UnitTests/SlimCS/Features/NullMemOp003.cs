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
		value?.TestDo();
		print("HERE.");
	}

	public static int TestDo(this int value)
	{
		print(value);
		return 1;
	}
}
