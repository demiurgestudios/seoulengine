using static SlimCS;

public static class Test
{
	public class A
	{
		public static bool a0 { get; set; } public static bool a1 { get; set; } = true; public static bool a2 { get; set; }
		public static double b0 { get; set; } public static double b1 { get; set; } = 1.0; public static double b2 { get; set; }
		public static int c0 { get; set; } public static int c1 { get; set; } = 1; public static int c2 { get; set; }
		public static string d0 { get; set; } public static string d1 { get; set; } = ""; public static string d2 { get; set; }
		public static B e0 { get; set; } public static B e1 { get; set; } = new B(); public static B e2 { get; set; }
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
