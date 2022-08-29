using static SlimCS;

public static class Test
{
	public class A
	{
		public static bool a0, a1 = true, a2;
		public static double b0, b1 = 1.0, b2;
		public static int c0, c1 = 1, c2;
		public static string d0, d1 = "", d2;
		public static B e0, e1 = new B(), e2;
	}

	public class B
	{
	}

	public static void Main ()
	{
		print(A.a0, A.a1, A.a2);
		print(A.b0, A.b1, A.b2);
		print(A.c0, A.c1, A.c2);
		print(A.d0, A.d1, A.d2);
		print(A.e0, A.e1, A.e2);
	}
}
