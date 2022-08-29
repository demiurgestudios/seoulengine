using static SlimCS;
public class Test
{
	public static int Main ()
	{
		var i = 5;
		var b = true;
		var s = "foobar";

		print(b);
		if (!b)
			return 1;
		print(i);
		if (i > 5)
			return 2;
		print(s);
		if (s != "foobar")
			return 3;

		return 0;
	}
}
