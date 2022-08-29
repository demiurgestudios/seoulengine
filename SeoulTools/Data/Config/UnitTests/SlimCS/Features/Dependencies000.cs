using static SlimCS;
using static A.A;
using B;

public static class Test
{
	public static void Main()
	{
		// Static members, static using.
		print(Enum.A);
		print(Enum.B);
		print(Field);
		print(Method(true));
		print(Method());
		print((new A.A()).Method(57));
		print((new A.A()).Method(new B.B()));
		print(Property);

		print(B.B.Enum.A);
		print(B.B.Enum.B);
		print(B.B.Field);
		print(B.B.Method(true));
		print(B.B.Method());
		print((new B.B()).Method(57));
		print((new B.B()).Method(new B.B()));
		print(B.B.Property);
		B.B.Property = 7019;
		print(B.B.Property);

		print(C.D.E.F.Enum.A);
		print(C.D.E.F.Enum.B);
		print(C.D.E.F.Field);
		print(C.D.E.F.Method(true));
		print(C.D.E.F.Method());
		print((new C.D.E.F()).Method(57));
		print((new C.D.E.F()).Method(new B.B()));
		print(C.D.E.F.Property);
		C.D.E.F.Property = 3999;
		print(B.B.Property);
		print(C.D.E.F.Property);
	}
}

namespace A
{
	class A
	{
		int m_i = 101;

		protected int MI { get { return m_i; } }

		public enum Enum
		{
			A = 5,
			B = 1,
		}

		public static string Field = "Hello World";
		public static int Method() { return 7; }
		public static int Method(bool b) { return (b ? 8 : 3); }
		public int Method(int i) { return i; }
		public virtual int Method(B.B b) { return b.I; }
		public static int Property { get; set; } = 5;
	}
}
