using static SlimCS;
public static class Test
{
	class A
	{
		internal int a = 1;
		internal A()
		{
			print("A");
			b = 7;
		}
		internal int b = 2;
		internal bool c = true;
	}

	public static void Main()
	{
		var a = new A(); print(a);
		print(a.a);
		print(a.b);
		print(a.c);
	}
}
