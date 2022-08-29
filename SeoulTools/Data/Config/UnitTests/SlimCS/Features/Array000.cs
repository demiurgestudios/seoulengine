using static SlimCS;
public static class Test
{
	static void Print(int[] a)
	{
		foreach (var i in a)
		{
			print(i);
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
		var a = new int[] { 7, 8, 9 };
		Print(a);

		a[1] = 10;
		Print(a);

		a[2] = 818;
		Print(a);
	}
}
