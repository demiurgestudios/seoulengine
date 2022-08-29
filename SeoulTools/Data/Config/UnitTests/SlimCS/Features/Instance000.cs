using static SlimCS;
public static class Test
{
	public sealed class A
	{
		public int a = 5;
	}

	public sealed class B
	{
		public A a = new A();
	}

	public static void Main()
	{
		var a = new B(); print(a.a); print(a.a.a);
		var b = new B(); print(b.a); print(b.a.a);

		a.a.a = 10;
		print(a.a);
		print(a.a.a);
		print(b.a);
		print(b.a.a);

		a.a = null;
		print(a.a);
		print(b.a);
		print(b.a.a);

		b.a = null;
		print(a.a);
		print(b.a);
	}
}
