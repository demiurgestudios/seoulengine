using static SlimCS;

public sealed class Test
{
	public delegate void TestDelegate(bool b);

	public static void Main ()
	{
		var tester = new Test();
		object o = (TestDelegate)tester.TestIt;
		((TestDelegate)o)(true);
		((TestDelegate)o)(false);

		tester.Test2();
	}

	public void Test2()
	{
		object o = (TestDelegate)TestIt;
		((TestDelegate)o)(true);
		((TestDelegate)o)(false);
	}

	public void TestIt(bool b)
	{
		print(b);
	}
}
