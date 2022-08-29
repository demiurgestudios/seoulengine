using static SlimCS;
internal class Test
{
	public static void Main ()
	{
		Method (1, 2, paramNamed: 3);
	}

	static void Method (int p1, int paramNamed, int p2)
	{
		print(p1, paramNamed, p2);
		throw new System.Exception ();
	}

	static void Method (int p1, int p2, object paramNamed)
	{
		print(p1, p2, paramNamed);
	}
}
