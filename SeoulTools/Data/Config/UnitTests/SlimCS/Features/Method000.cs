using static SlimCS;
public static class Test
{
	static int m = 1;

	class a
	{
		internal int m = 0;
		public int b() { return m; }
		public int c() { return this.m; }
	}

	public static void Main()
	{
		var a = new a();
		print(m);
		var v = a.b();
		print(v);
		v = a.c();
		print(v);

		a.m = 2;
		v = a.b();
		print(v);
		v = a.c();
		print(v);

		a = new a();
		v = a.b();
		print(v);
		v = a.c();
		print(v);
	}
}
