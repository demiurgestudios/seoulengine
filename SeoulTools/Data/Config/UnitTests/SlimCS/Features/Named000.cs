using static SlimCS;
class A
{
	public int id;

	public int this [int i] {
		set { print("set= " + i); id = value; }
		get { print("get= " + i); return id; }
	}
}

sealed class MyPoint
{
	public MyPoint (int x, int y)
	{
		print(x, y);
		X = x;
		Y = y;
	}

	public int X, Y;
}

class Test
{
	static double Foo (double t, double a)
	{
		print(t, a);
		return a;
	}

	static string Bar (int a = 1, string s = "2", int c = '3')
	{
		print(a, s, c);
		return a.ToString () + s + c;
	}

	static int Test1 (int a, int b)
	{
		print(a, b);
		return a * 3 + b * 7;
	}

	public static int Main ()
	{
		int h = 9;
		if (Foo (a : h, t : 3) != 9)
			return 1;

		if (Bar (a : 1, s : "x", c : '2') != "1x50")
			return 3;

		if (Bar (s : "x") != "1x51")
			return 4;

		int i = 1;
		if (Test1 (a: i, b: i+1) != 17)
			return 5;

		i = 1;
		if (Test1 (b: i, a: i+1) != 13)
			return 7;

		A a = new A ();
		i = 5;
		a [i:i+1]++;

		if (a.id != 1)
			return 8;

		MyPoint mp = new MyPoint (y : -1, x : 5);
		if (mp.Y != -1)
			return 10;

		print("ok");
		return 0;
	}
}