using static SlimCS;
using N0.N1.N2;
using static Test;

public static class Test
{
	public class Y
	{
		public X A { get; set; }
		X x; public X B { get { return x; } set { x = value; } }
	}

	public static void Main()
	{
		var x = new X(); print(x.A, x.B);
		x.A = new Y(); print(x.A, x.B, x.A.A, x.A.B);
		x.B = new Y(); print(x.A, x.B, x.A.A, x.A.B, x.B.A, x.B.B);
		x.A.A = new X(); print(x.A, x.B, x.A.A, x.A.B, x.B.A, x.B.B);
		x.A.B = new X(); print(x.A, x.B, x.A.A, x.A.B, x.B.A, x.B.B);
		x.B.A = new X(); print(x.A, x.B, x.A.A, x.A.B, x.B.A, x.B.B);
		x.B.B = new X(); print(x.A, x.B, x.A.A, x.A.B, x.B.A, x.B.B);
	}
}

namespace N0.N1.N2
{
	public class X
	{
		public Y A { get; set; }
		Y y; public Y B { get { return y; } set { y = value; } }
	}
}
