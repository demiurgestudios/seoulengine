using static SlimCS;
public class X
{
	int a; public int A { get { print("get_A"); return a; } set { print("set_A"); a = value; } }
	bool b; public bool B { get { print("get_B"); return b; } set { print("set_B"); b = value; } }
	string c; public string C { get { print("get_C"); return c; } set { print("set_C"); c = value; } }
	int d; public int D { get { print("get_D"); return d; } set { print("set_D"); d = value; } }
}

public static class Test
{
	public static void Main()
	{
		var a = new X()
		{
			D = -25,
			C = "Hello World",
			A = 132,
		};
		print(a.A);
		print(a.B);
		print(a.C);
		print(a.D);
	}
}
