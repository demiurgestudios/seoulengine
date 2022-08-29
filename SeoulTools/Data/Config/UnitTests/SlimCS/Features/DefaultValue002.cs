using static SlimCS;

public static class Test
{
	public class A
	{
		public bool a0 { get; set; } public bool a1 { get; set; } public bool a2 { get; set; }
		public double b0 { get; set; } public double b1 { get; set; } public double b2 { get; set; }
		public int c0 { get; set; } public int c1 { get; set; } public int c2 { get; set; }
		public string d0 { get; set; } public string d1 { get; set; } public string d2 { get; set; }
		public B e0 { get; set; } public B e1 { get; set; } public B e2 { get; set; }
	}

	public class B
	{
	}

	public static void Main ()
	{
		var a = new A();
		print(a.a0, a.a1, a.a2);
		print(a.b0, a.b1, a.b2);
		print(a.c0, a.c1, a.c2);
		print(a.d0, a.d1, a.d2);
		print(a.e0, a.e1, a.e2);
	}
}
