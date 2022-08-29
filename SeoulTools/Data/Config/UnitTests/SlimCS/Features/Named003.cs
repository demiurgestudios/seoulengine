using static SlimCS;
class Test
{
	public static int Main ()
	{
		var d = new Test();
		if (d.Foo (x: 1, y : 2) != 3)
			return 1;

		return 0;
	}

	public int Foo (int x, double y, string a = "a")
	{
		print(x, y, a);
		return 1;
	}

	public int Foo (int x, double y, params string[] args)
	{
		print(x, y, args);
		return 2;
	}

	public int Foo (double y, int x)
	{
		print(y, x);
		return 3;
	}
}
