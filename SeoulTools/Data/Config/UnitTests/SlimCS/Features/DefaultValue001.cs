using static SlimCS;

public static class Test
{
	public class A
	{
		public bool a0, a1 = true, a2;
		public double b0, b1 = 1.0, b2;
		public int c0, c1 = 1, c2;
		public string d0, d1 = "", d2;
		public B e0, e1 = new B(), e2;
	}

	public class B
	{
	}

	public static void Main()
	{
		var a = new A();
		print(a.a0, a.a1, a.a2);
		print(a.b0, a.b1, a.b2);
		print(a.c0, a.c1, a.c2);
		print(a.d0, a.d1, a.d2);
		print(a.e0, a.e1, a.e2);
	}
}
