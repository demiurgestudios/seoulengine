using static SlimCS;
public static class Test
{
	public static void Main()
	{
		TestIt();
	}

	public static void TestIt()
	{
		int? value = null;
		value?.TestDo();
		print("HERE.");
	}

	public static void TestDo(this int value)
	{
		print(value);
	}
}
