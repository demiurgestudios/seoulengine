using static SlimCS;
public static class Test
{
	static void A(params int[] a)
	{
		foreach (var i in a)
		{
			print(i);
		}
	}

	static void B(params bool[] a)
	{
		print(a[0]);
		print(a[1]);
		print(a[2]);
	}

	static void C(params int[] a)
	{
		var b = a;
		foreach (var i in b)
		{
			print(i);
		}
		A(a);
	}

	static void D(int a, int b, params int[] c)
	{
		print(a);
		print(b);
		C(c);
	}

	public static void Main()
	{
		A(new [] { 3, 4, 5 });
		B(new [] { true, false, true });
		C(new [] { 7, 8 });
		D(9, 10, new[] { 11, 12 });

		A(3, 4, 5 );
		B(true, false, true );
		C(7, 8);
		D(9, 10, 11, 12 );
	}
}
