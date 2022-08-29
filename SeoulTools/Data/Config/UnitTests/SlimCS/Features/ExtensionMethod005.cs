using static SlimCS;
using System;

class S
{
	public string Prop
	{
		get
		{
			print("5");
			return "5";
		}
	}
}

class S2
{
	public bool Prop;
}

static class E
{
	public static int Prop (this S s)
	{
		print(8);
		return 8;
	}

	public static int Prop (this S2 s)
	{
		print(18);
		return 18;
	}
}

class Test
{
	public static void Main()
	{
		S s = new S ();
		int b = s.Prop ();
		print(b);
		string bb = s.Prop;
		print(bb);

		S2 s2 = new S2 ();
		int b2 = s2.Prop ();
		print(b2);
		bool bb2 = s2.Prop;
		print(bb2);
	}
}
