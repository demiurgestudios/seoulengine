using static SlimCS;
class Test
{
	static int Foo (object arg, bool b1 = false, bool b2 = false, bool b3 = true, object o2 = null)
	{
		print(arg, b1, b2, b3, o2);

		if ((int) arg != 1)
			return 1;

		if (b1)
			return 2;

		if (b2)
			return 3;

		if (!b3)
			return 4;

		if ((string) o2 != "s")
			return 5;

		return 0;
	}

	public static int Main ()
	{
		object o = "s"; print(o);
		return Foo (1, b3 : true, o2 : o);
	}
}
