using static SlimCS;
public static class Test
{
	class A {}

	static void Print(A[] a)
	{
		foreach (var x in a)
		{
			print(x);
		}

		for (int i = 0; i < a.Length; ++i)
		{
			print(a[i]);
		}

		print(a[0]);
		print(a[1]);
		print(a[2]);
	}

	public static void Main()
	{
		var a = new [] { null, new A(), null };
		Print(a);

		a[1] = null;
		Print(a);

		a[2] = new A();
		Print(a);

		a[0] = new A();
		Print(a);
	}
}
