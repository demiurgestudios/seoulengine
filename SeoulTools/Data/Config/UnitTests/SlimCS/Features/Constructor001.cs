using static SlimCS;
public static class Test
{
	public static void Main()
	{
		var a = new A(31, 9, false); print(a);
		print(a.a);
		print(a.b);
		print(a.c);
	}

	class A
	{
		internal int a = 1;
		internal A(
			int a,
			int b,
			bool c)
		{
			print("A");
			b = 7;
			this.a = a;
			this.b = b;
			this.c = c;
		}
		internal int b = 2;
		internal bool c = true;
	}
}
