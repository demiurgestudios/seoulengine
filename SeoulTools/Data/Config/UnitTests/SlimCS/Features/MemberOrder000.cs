using static SlimCS;
public static class Test
{
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

	class a
	{
		public int b() { return m; }
		internal int m = 0;
		public int c() { return this.m; }
	}

	static int m = 1;
}
