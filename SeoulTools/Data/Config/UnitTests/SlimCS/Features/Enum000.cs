using static SlimCS;
public enum A { One, Two };

public static class Test
{
	public static void Main()
	{
		print((int)A.One);
		print((int)A.Two);

		var e = A.One;
		// TODO: Tricky, easy if everything is typed,
		// not when it's global. print(e);
		e = A.Two;
		// TODO: Tricky, easy if everything is typed,
		// not when it's global. print(e);
	}
}
