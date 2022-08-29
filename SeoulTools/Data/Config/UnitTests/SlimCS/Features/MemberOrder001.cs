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
		a.setx(11);

		var b = new b();
		print(b); print(b.y);

		a = new a();
		v = a.b();
		b.y = 15;
		print(v);
		print(b); print(b.y);
		v = a.c();
		print(v);
		v = b.y;
		print(v);
	}

	class a
	{
		public int b() { return x; }
		internal int m = 0;
		public void setx(int x) { this.x = x; }
		public int c() { return this.m; }
		int x = 7;
	}

	static int m = 1;
}

class b
{
	public int y = 10;
}
