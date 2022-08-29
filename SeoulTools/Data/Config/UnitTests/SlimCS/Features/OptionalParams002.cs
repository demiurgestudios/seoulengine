using static SlimCS;

public static class Test
{
	const string kDefault = "default";

	static void TestIt(string s = kDefault, bool b = true)
	{
		print(s, b);
	}

	static void TestIt2(string s = TestValue.kDefault2, bool b = true)
	{
		print(s, b);
	}

	public static void Main()
	{
		TestIt("Hello World");
		TestIt("Hello World", b: false);
		TestIt();
		TestIt(b: false);
		TestIt2();
		TestIt2(b: false);
		TestIt2("Hi There");
		TestIt2("Hi There", b: false);
		TestIt2();
		TestIt2(b: false);
	}
}

public static class TestValue
{
	public const string kDefault2 = "default2";
}
