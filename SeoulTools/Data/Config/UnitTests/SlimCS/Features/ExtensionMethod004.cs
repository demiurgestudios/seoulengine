using static SlimCS;
using System;

static class S
{
	public static void Extension(this A b, string s, bool n)
	{
		throw new Exception ("wrong overload");
	}
}

class A
{
	public void Extension (string s)
	{
		print(s);
	}
}

public static class Test
{
	static void TestMethod(Action<bool> arg)
	{
	}

	static int TestMethod(Action<string> arg)
	{
		arg("here");
		return 2;
	}

	public static int Main()
	{
		var a = new A ();
		if (TestMethod (a.Extension) != 2)
			return 1;

		return 0;
	}
}
